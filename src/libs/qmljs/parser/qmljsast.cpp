/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qmljsast_p.h"

#include "qmljsastvisitor_p.h"

QT_QML_BEGIN_NAMESPACE

namespace QmlJS { namespace AST {

FunctionExpression *asAnonymousFunctionDefinition(Node *n)
{
    if (!n)
        return nullptr;
    FunctionExpression *f = n->asFunctionDefinition();
    if (!f || !f->name.isNull())
        return nullptr;
    return f;
}

ClassExpression *asAnonymousClassDefinition(Node *n)
{
    if (!n)
        return nullptr;
    ClassExpression *c = n->asClassDefinition();
    if (!c || !c->name.isNull())
        return nullptr;
    return c;
}


void Node::accept(Visitor *visitor)
{
    if (visitor->preVisit(this)) {
        accept0(visitor);
    }
    visitor->postVisit(this);
}

void Node::accept(Node *node, Visitor *visitor)
{
    if (node)
        node->accept(visitor);
}

ExpressionNode *Node::expressionCast()
{
    return nullptr;
}

BinaryExpression *Node::binaryExpressionCast()
{
    return nullptr;
}

Statement *Node::statementCast()
{
    return nullptr;
}

UiObjectMember *Node::uiObjectMemberCast()
{
    return nullptr;
}

LeftHandSideExpression *Node::leftHandSideExpressionCast()
{
    return nullptr;
}

Pattern *Node::patternCast()
{
    return nullptr;
}

FunctionExpression *Node::asFunctionDefinition()
{
    return nullptr;
}

ClassExpression *Node::asClassDefinition()
{
    return nullptr;
}

ExpressionNode *ExpressionNode::expressionCast()
{
    return this;
}

FormalParameterList *ExpressionNode::reparseAsFormalParameterList(MemoryPool *pool)
{
    AST::ExpressionNode *expr = this;
    AST::FormalParameterList *f = nullptr;
    if (AST::Expression *commaExpr = AST::cast<AST::Expression *>(expr)) {
        f = commaExpr->left->reparseAsFormalParameterList(pool);
        if (!f)
            return nullptr;

        expr = commaExpr->right;
    }

    AST::ExpressionNode *rhs = nullptr;
    if (AST::BinaryExpression *assign = AST::cast<AST::BinaryExpression *>(expr)) {
            if (assign->op != QSOperator::Assign)
                return nullptr;
        expr = assign->left;
        rhs = assign->right;
    }
    AST::PatternElement *binding = nullptr;
    if (AST::IdentifierExpression *idExpr = AST::cast<AST::IdentifierExpression *>(expr)) {
        binding = new (pool) AST::PatternElement(idExpr->name, rhs);
        binding->identifierToken = idExpr->identifierToken;
    } else if (AST::Pattern *p = expr->patternCast()) {
        SourceLocation loc;
        QString s;
        if (!p->convertLiteralToAssignmentPattern(pool, &loc, &s))
            return nullptr;
        binding = new (pool) AST::PatternElement(p, rhs);
        binding->identifierToken = p->firstSourceLocation();
    }
    if (!binding)
        return nullptr;
    return new (pool) AST::FormalParameterList(f, binding);
}

BinaryExpression *BinaryExpression::binaryExpressionCast()
{
    return this;
}

Statement *Statement::statementCast()
{
    return this;
}

UiObjectMember *UiObjectMember::uiObjectMemberCast()
{
    return this;
}

void NestedExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }
    visitor->endVisit(this);
}

FunctionExpression *NestedExpression::asFunctionDefinition()
{
    return expression->asFunctionDefinition();
}

ClassExpression *NestedExpression::asClassDefinition()
{
    return expression->asClassDefinition();
}

void ThisExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void IdentifierExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void NullExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void TrueLiteral::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void FalseLiteral::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void SuperLiteral::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}


void StringLiteral::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void TemplateLiteral::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        if (next)
            accept(next, visitor);
    }

    visitor->endVisit(this);
}

void NumericLiteral::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void RegExpLiteral::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void ArrayPattern::accept0(Visitor *visitor)
{
    if (visitor->visit(this))
        accept(elements, visitor);

    visitor->endVisit(this);
}

bool ArrayPattern::isValidArrayLiteral(SourceLocation *errorLocation) const {
    for (PatternElementList *it = elements; it != nullptr; it = it->next) {
        PatternElement *e = it->element;
        if (e && e->bindingTarget != nullptr) {
            if (errorLocation)
                *errorLocation = e->firstSourceLocation();
            return false;
        }
    }
    return true;
}

void ObjectPattern::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(properties, visitor);
    }

    visitor->endVisit(this);
}

/*
  This is the grammar for AssignmentPattern that we need to convert the literal to:

    AssignmentPattern:
        ObjectAssignmentPattern
        ArrayAssignmentPattern
    ArrayAssignmentPattern:
        [ ElisionOpt AssignmentRestElementOpt ]
        [ AssignmentElementList ]
        [ AssignmentElementList , ElisionOpt AssignmentRestElementOpt ]
    AssignmentElementList:
        AssignmentElisionElement
        AssignmentElementList , AssignmentElisionElement
    AssignmentElisionElement:
        ElisionOpt AssignmentElement
    AssignmentRestElement:
        ... DestructuringAssignmentTarget

    ObjectAssignmentPattern:
        {}
        { AssignmentPropertyList }
        { AssignmentPropertyList, }
    AssignmentPropertyList:
        AssignmentProperty
        AssignmentPropertyList , AssignmentProperty
    AssignmentProperty:
        IdentifierReference InitializerOpt_In
    PropertyName:
        AssignmentElement

    AssignmentElement:
        DestructuringAssignmentTarget InitializerOpt_In
    DestructuringAssignmentTarget:
        LeftHandSideExpression

  It was originally parsed with the following grammar:

ArrayLiteral:
    [ ElisionOpt ]
    [ ElementList ]
    [ ElementList , ElisionOpt ]
ElementList:
    ElisionOpt AssignmentExpression_In
    ElisionOpt SpreadElement
    ElementList , ElisionOpt AssignmentExpression_In
    ElementList , Elisionopt SpreadElement
SpreadElement:
    ... AssignmentExpression_In
ObjectLiteral:
    {}
    { PropertyDefinitionList }
    { PropertyDefinitionList , }
PropertyDefinitionList:
    PropertyDefinition
    PropertyDefinitionList , PropertyDefinition
PropertyDefinition:
    IdentifierReference
    CoverInitializedName
    PropertyName : AssignmentExpression_In
    MethodDefinition
PropertyName:
    LiteralPropertyName
    ComputedPropertyName

*/
bool ArrayPattern::convertLiteralToAssignmentPattern(MemoryPool *pool, SourceLocation *errorLocation, QString *errorMessage)
{
    if (parseMode == Binding)
        return true;
    for (auto *it = elements; it; it = it->next) {
        if (!it->element)
            continue;
        if (it->element->type == PatternElement::SpreadElement && it->next) {
            *errorLocation = it->element->firstSourceLocation();
            *errorMessage = QString::fromLatin1("'...' can only appear as last element in a destructuring list.");
            return false;
        }
        if (!it->element->convertLiteralToAssignmentPattern(pool, errorLocation, errorMessage))
            return false;
    }
    parseMode = Binding;
    return true;
}

bool ObjectPattern::convertLiteralToAssignmentPattern(MemoryPool *pool, SourceLocation *errorLocation, QString *errorMessage)
{
    if (parseMode == Binding)
        return true;
    for (auto *it = properties; it; it = it->next) {
        if (!it->property->convertLiteralToAssignmentPattern(pool, errorLocation, errorMessage))
            return false;
    }
    parseMode = Binding;
    return true;
}

bool PatternElement::convertLiteralToAssignmentPattern(MemoryPool *pool, SourceLocation *errorLocation, QString *errorMessage)
{
    Q_ASSERT(type == Literal || type == SpreadElement);
    Q_ASSERT(bindingIdentifier.isNull());
    Q_ASSERT(bindingTarget == nullptr);
    Q_ASSERT(bindingTarget == nullptr);
    Q_ASSERT(initializer);
    ExpressionNode *init = initializer;

    initializer = nullptr;
    LeftHandSideExpression *lhs = init->leftHandSideExpressionCast();
    if (type == SpreadElement) {
        if (!lhs) {
            *errorLocation = init->firstSourceLocation();
            *errorMessage = QString::fromLatin1("Invalid lhs expression after '...' in destructuring expression.");
            return false;
        }
    } else {
        type = PatternElement::Binding;

        if (BinaryExpression *b = init->binaryExpressionCast()) {
            if (b->op != QSOperator::Assign) {
                *errorLocation = b->operatorToken;
                *errorMessage = QString::fromLatin1("Invalid assignment operation in destructuring expression");
                return false;
            }
            lhs = b->left->leftHandSideExpressionCast();
            initializer = b->right;
            Q_ASSERT(lhs);
        } else {
            lhs = init->leftHandSideExpressionCast();
        }
        if (!lhs) {
            *errorLocation = init->firstSourceLocation();
            *errorMessage = QString::fromLatin1("Destructuring target is not a left hand side expression.");
            return false;
        }
    }

    if (auto *i = cast<IdentifierExpression *>(lhs)) {
        bindingIdentifier = i->name;
        identifierToken = i->identifierToken;
        return true;
    }

    bindingTarget = lhs;
    if (auto *p = lhs->patternCast()) {
        if (!p->convertLiteralToAssignmentPattern(pool, errorLocation, errorMessage))
            return false;
    }
    return true;
}

bool PatternProperty::convertLiteralToAssignmentPattern(MemoryPool *pool, SourceLocation *errorLocation, QString *errorMessage)
{
    Q_ASSERT(type != SpreadElement);
    if (type == Binding)
        return true;
    if (type == Getter || type == Setter) {
        *errorLocation = firstSourceLocation();
        *errorMessage = QString::fromLatin1("Invalid getter/setter in destructuring expression.");
        return false;
    }
    Q_ASSERT(type == Literal);
    return PatternElement::convertLiteralToAssignmentPattern(pool, errorLocation, errorMessage);
}


void Elision::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        // ###
    }

    visitor->endVisit(this);
}

void IdentifierPropertyName::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void StringLiteralPropertyName::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void NumericLiteralPropertyName::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

namespace {
struct LocaleWithoutZeroPadding : public QLocale
{
    LocaleWithoutZeroPadding()
        : QLocale(QLocale::C)
    {
        setNumberOptions(QLocale::OmitLeadingZeroInExponent | QLocale::OmitGroupSeparator);
    }
};
}

QString NumericLiteralPropertyName::asString()const
{
    // Can't use QString::number here anymore as it does zero padding by default now.

    // In C++11 this initialization is thread-safe (6.7 [stmt.dcl] p4)
    static LocaleWithoutZeroPadding locale;
    // Because of https://gcc.gnu.org/bugzilla/show_bug.cgi?id=83562 we can't use thread_local
    // for the locale variable and therefore rely on toString(double) to be thread-safe.
    return locale.toString(id, 'g', 16);
}

void ArrayMemberExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(base, visitor);
        accept(expression, visitor);
    }

    visitor->endVisit(this);
}

void FieldMemberExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(base, visitor);
    }

    visitor->endVisit(this);
}

void NewMemberExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(base, visitor);
        accept(arguments, visitor);
    }

    visitor->endVisit(this);
}

void NewExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }

    visitor->endVisit(this);
}

void CallExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(base, visitor);
        accept(arguments, visitor);
    }

    visitor->endVisit(this);
}

void ArgumentList::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        for (ArgumentList *it = this; it; it = it->next) {
            accept(it->expression, visitor);
        }
    }

    visitor->endVisit(this);
}

void PostIncrementExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(base, visitor);
    }

    visitor->endVisit(this);
}

void PostDecrementExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(base, visitor);
    }

    visitor->endVisit(this);
}

void DeleteExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }

    visitor->endVisit(this);
}

void VoidExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }

    visitor->endVisit(this);
}

void TypeOfExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }

    visitor->endVisit(this);
}

void PreIncrementExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }

    visitor->endVisit(this);
}

void PreDecrementExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }

    visitor->endVisit(this);
}

void UnaryPlusExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }

    visitor->endVisit(this);
}

void UnaryMinusExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }

    visitor->endVisit(this);
}

void TildeExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }

    visitor->endVisit(this);
}

void NotExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }

    visitor->endVisit(this);
}

void BinaryExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(left, visitor);
        accept(right, visitor);
    }

    visitor->endVisit(this);
}

void ConditionalExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
        accept(ok, visitor);
        accept(ko, visitor);
    }

    visitor->endVisit(this);
}

void Expression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(left, visitor);
        accept(right, visitor);
    }

    visitor->endVisit(this);
}

void Block::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(statements, visitor);
    }

    visitor->endVisit(this);
}

void StatementList::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        for (StatementList *it = this; it; it = it->next) {
            accept(it->statement, visitor);
        }
    }

    visitor->endVisit(this);
}

void VariableStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(declarations, visitor);
    }

    visitor->endVisit(this);
}

void VariableDeclarationList::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        for (VariableDeclarationList *it = this; it; it = it->next) {
            accept(it->declaration, visitor);
        }
    }

    visitor->endVisit(this);
}

void EmptyStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void ExpressionStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }

    visitor->endVisit(this);
}

void IfStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
        accept(ok, visitor);
        accept(ko, visitor);
    }

    visitor->endVisit(this);
}

void DoWhileStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(statement, visitor);
        accept(expression, visitor);
    }

    visitor->endVisit(this);
}

void WhileStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
        accept(statement, visitor);
    }

    visitor->endVisit(this);
}

void ForStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(initialiser, visitor);
        accept(declarations, visitor);
        accept(condition, visitor);
        accept(expression, visitor);
        accept(statement, visitor);
    }

    visitor->endVisit(this);
}

void ForEachStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(lhs, visitor);
        accept(expression, visitor);
        accept(statement, visitor);
    }

    visitor->endVisit(this);
}

void ContinueStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void BreakStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void ReturnStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }

    visitor->endVisit(this);
}

void YieldExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }

    visitor->endVisit(this);
}


void WithStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
        accept(statement, visitor);
    }

    visitor->endVisit(this);
}

void SwitchStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
        accept(block, visitor);
    }

    visitor->endVisit(this);
}

void CaseBlock::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(clauses, visitor);
        accept(defaultClause, visitor);
        accept(moreClauses, visitor);
    }

    visitor->endVisit(this);
}

void CaseClauses::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        for (CaseClauses *it = this; it; it = it->next) {
            accept(it->clause, visitor);
        }
    }

    visitor->endVisit(this);
}

void CaseClause::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
        accept(statements, visitor);
    }

    visitor->endVisit(this);
}

void DefaultClause::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(statements, visitor);
    }

    visitor->endVisit(this);
}

void LabelledStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(statement, visitor);
    }

    visitor->endVisit(this);
}

void ThrowStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }

    visitor->endVisit(this);
}

void TryStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(statement, visitor);
        accept(catchExpression, visitor);
        accept(finallyExpression, visitor);
    }

    visitor->endVisit(this);
}

void Catch::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(patternElement, visitor);
        accept(statement, visitor);
    }

    visitor->endVisit(this);
}

void Finally::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(statement, visitor);
    }

    visitor->endVisit(this);
}

void FunctionDeclaration::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(formals, visitor);
        accept(body, visitor);
    }

    visitor->endVisit(this);
}

void FunctionExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(formals, visitor);
        accept(body, visitor);
    }

    visitor->endVisit(this);
}

FunctionExpression *FunctionExpression::asFunctionDefinition()
{
    return this;
}

QStringList FormalParameterList::formals() const
{
    QStringList formals;
    int i = 0;
    for (const FormalParameterList *it = this; it; it = it->next) {
        if (it->element) {
            QString name = it->element->bindingIdentifier.toString();
            int duplicateIndex = formals.indexOf(name);
            if (duplicateIndex >= 0) {
                // change the name of the earlier argument to enforce the lookup semantics from the spec
                formals[duplicateIndex] += QLatin1String("#") + QString::number(i);
            }
            formals += name;
        }
        ++i;
    }
    return formals;
}

QStringList FormalParameterList::boundNames() const
{
    QStringList names;
    for (const FormalParameterList *it = this; it; it = it->next) {
        if (it->element)
            it->element->boundNames(&names);
    }
    return names;
}

void FormalParameterList::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(element, visitor);
        if (next)
            accept(next, visitor);
    }

    visitor->endVisit(this);
}

FormalParameterList *FormalParameterList::finish(QmlJS::MemoryPool *pool)
{
    FormalParameterList *front = next;
    next = nullptr;

    int i = 0;
    for (const FormalParameterList *it = this; it; it = it->next) {
        if (it->element && it->element->bindingIdentifier.isEmpty())
            it->element->bindingIdentifier = pool->newString(QLatin1String("arg#") + QString::number(i));
        ++i;
    }
    return front;
}

void Program::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(statements, visitor);
    }

    visitor->endVisit(this);
}

void ImportSpecifier::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {

    }
    visitor->endVisit(this);
}

void ImportsList::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        for (ImportsList *it = this; it; it = it->next) {
            accept(it->importSpecifier, visitor);
        }
    }

    visitor->endVisit(this);
}

void NamedImports::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(importsList, visitor);
    }

    visitor->endVisit(this);
}

void FromClause::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void NameSpaceImport::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void ImportClause::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(nameSpaceImport, visitor);
        accept(namedImports, visitor);
    }

    visitor->endVisit(this);
}

void ImportDeclaration::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(importClause, visitor);
        accept(fromClause, visitor);
    }

    visitor->endVisit(this);
}

void ExportSpecifier::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {

    }

    visitor->endVisit(this);
}

void ExportsList::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        for (ExportsList *it = this; it; it = it->next) {
            accept(it->exportSpecifier, visitor);
        }
    }

    visitor->endVisit(this);
}

void ExportClause::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(exportsList, visitor);
    }

    visitor->endVisit(this);
}

void ExportDeclaration::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(fromClause, visitor);
        accept(exportClause, visitor);
        accept(variableStatementOrDeclaration, visitor);
    }

    visitor->endVisit(this);
}

void ESModule::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(body, visitor);
    }

    visitor->endVisit(this);
}

void DebuggerStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void UiProgram::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(headers, visitor);
        accept(members, visitor);
    }

    visitor->endVisit(this);
}

void UiPublicMember::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(statement, visitor);
        accept(binding, visitor);
    }

    visitor->endVisit(this);
}

void UiObjectDefinition::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(qualifiedTypeNameId, visitor);
        accept(initializer, visitor);
    }

    visitor->endVisit(this);
}

void UiObjectInitializer::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(members, visitor);
    }

    visitor->endVisit(this);
}

void UiParameterList::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }
    visitor->endVisit(this);
}

void UiObjectBinding::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(qualifiedId, visitor);
        accept(qualifiedTypeNameId, visitor);
        accept(initializer, visitor);
    }

    visitor->endVisit(this);
}

void UiScriptBinding::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(qualifiedId, visitor);
        accept(statement, visitor);
    }

    visitor->endVisit(this);
}

void UiArrayBinding::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(qualifiedId, visitor);
        accept(members, visitor);
    }

    visitor->endVisit(this);
}

void UiObjectMemberList::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        for (UiObjectMemberList *it = this; it; it = it->next)
            accept(it->member, visitor);
    }

    visitor->endVisit(this);
}

void UiArrayMemberList::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        for (UiArrayMemberList *it = this; it; it = it->next)
            accept(it->member, visitor);
    }

    visitor->endVisit(this);
}

void UiQualifiedId::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void UiImport::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(importUri, visitor);
    }

    visitor->endVisit(this);
}

void UiPragma::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void UiHeaderItemList::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(headerItem, visitor);
        accept(next, visitor);
    }

    visitor->endVisit(this);
}


void UiSourceElement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(sourceElement, visitor);
    }

    visitor->endVisit(this);
}

void UiEnumDeclaration::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(members, visitor);
    }

    visitor->endVisit(this);
}

void UiEnumMemberList::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void TaggedTemplate::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(base, visitor);
        accept(templateLiteral, visitor);
    }

    visitor->endVisit(this);
}

void PatternElement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(bindingTarget, visitor);
        accept(initializer, visitor);
    }

    visitor->endVisit(this);
}

void PatternElement::boundNames(QStringList *names)
{
    if (bindingTarget) {
        if (PatternElementList *e = elementList())
            e->boundNames(names);
        else if (PatternPropertyList *p = propertyList())
            p->boundNames(names);
    } else {
        names->append(bindingIdentifier.toString());
    }
}

void PatternElementList::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(elision, visitor);
        accept(element, visitor);
        if (next)
            accept(next, visitor);
    }

    visitor->endVisit(this);
}

void PatternElementList::boundNames(QStringList *names)
{
    for (PatternElementList *it = this; it; it = it->next) {
        if (it->element)
            it->element->boundNames(names);
    }
}

void PatternProperty::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(name, visitor);
        accept(bindingTarget, visitor);
        accept(initializer, visitor);
    }

    visitor->endVisit(this);
}

void PatternProperty::boundNames(QStringList *names)
{
    PatternElement::boundNames(names);
}

void PatternPropertyList::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(property, visitor);
        if (next)
            accept(next, visitor);
    }

    visitor->endVisit(this);
}

void PatternPropertyList::boundNames(QStringList *names)
{
    for (PatternPropertyList *it = this; it; it = it->next)
        it->property->boundNames(names);
}

void ComputedPropertyName::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }

    visitor->endVisit(this);
}

void ClassExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(heritage, visitor);
        accept(elements, visitor);
    }

    visitor->endVisit(this);
}

ClassExpression *ClassExpression::asClassDefinition()
{
    return this;
}

void ClassDeclaration::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(heritage, visitor);
        accept(elements, visitor);
    }

    visitor->endVisit(this);
}

void ClassElementList::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(property, visitor);
        if (next)
            accept(next, visitor);
    }

    visitor->endVisit(this);
}

ClassElementList *ClassElementList::finish()
{
    ClassElementList *front = next;
    next = nullptr;
    return front;
}

Pattern *Pattern::patternCast()
{
    return this;
}

LeftHandSideExpression *LeftHandSideExpression::leftHandSideExpressionCast()
{
    return this;
}

} } // namespace QmlJS::AST

QT_QML_END_NAMESPACE


