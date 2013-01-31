/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmljscheck.h"
#include "qmljsbind.h"
#include "qmljscontext.h"
#include "qmljsevaluate.h"
#include "qmljsutils.h"
#include "parser/qmljsast_p.h"

#include <utils/qtcassert.h>

#include <QDebug>
#include <QDir>
#include <QColor>
#include <QApplication>

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace QmlJS::StaticAnalysis;

namespace {

class AssignmentCheck : public ValueVisitor
{
public:
    Message operator()(
            const Document::Ptr &document,
            const SourceLocation &location,
            const Value *lhsValue,
            const Value *rhsValue,
            Node *ast)
    {
        _doc = document;
        _rhsValue = rhsValue;
        _location = location;
        if (ExpressionStatement *expStmt = cast<ExpressionStatement *>(ast))
            _ast = expStmt->expression;
        else
            _ast = ast->expressionCast();

        if (lhsValue)
            lhsValue->accept(this);

        return _message;
    }

    void setMessage(Type type)
    {
        _message = Message(type, _location);
    }

    virtual void visit(const NumberValue *value)
    {
        if (const QmlEnumValue *enumValue = value_cast<QmlEnumValue>(value)) {
            if (StringLiteral *stringLiteral = cast<StringLiteral *>(_ast)) {
                const QString valueName = stringLiteral->value.toString();

                if (!enumValue->keys().contains(valueName))
                    setMessage(ErrInvalidEnumValue);
            } else if (! _rhsValue->asStringValue() && ! _rhsValue->asNumberValue()
                       && ! _rhsValue->asUnknownValue()) {
                setMessage(ErrEnumValueMustBeStringOrNumber);
            }
        } else {
            if (cast<TrueLiteral *>(_ast)
                    || cast<FalseLiteral *>(_ast)) {
                setMessage(ErrNumberValueExpected);
            }
        }
    }

    virtual void visit(const BooleanValue *)
    {
        UnaryMinusExpression *unaryMinus = cast<UnaryMinusExpression *>(_ast);

        if (cast<StringLiteral *>(_ast)
                || cast<NumericLiteral *>(_ast)
                || (unaryMinus && cast<NumericLiteral *>(unaryMinus->expression))) {
            setMessage(ErrBooleanValueExpected);
        }
    }

    virtual void visit(const StringValue *value)
    {
        UnaryMinusExpression *unaryMinus = cast<UnaryMinusExpression *>(_ast);

        if (cast<NumericLiteral *>(_ast)
                || (unaryMinus && cast<NumericLiteral *>(unaryMinus->expression))
                || cast<TrueLiteral *>(_ast)
                || cast<FalseLiteral *>(_ast)) {
            setMessage(ErrStringValueExpected);
        }

        if (value && value->asUrlValue()) {
            if (StringLiteral *literal = cast<StringLiteral *>(_ast)) {
                QUrl url(literal->value.toString());
                if (!url.isValid() && !url.isEmpty()) {
                    setMessage(ErrInvalidUrl);
                } else {
                    QString fileName = url.toLocalFile();
                    if (!fileName.isEmpty()) {
                        if (QFileInfo(fileName).isRelative()) {
                            fileName.prepend(QDir::separator());
                            fileName.prepend(_doc->path());
                        }
                        if (!QFileInfo(fileName).exists())
                            setMessage(WarnFileOrDirectoryDoesNotExist);
                    }
                }
            }
        }
    }

    virtual void visit(const ColorValue *)
    {
        if (StringLiteral *stringLiteral = cast<StringLiteral *>(_ast)) {
            if (!toQColor(stringLiteral->value.toString()).isValid())
                setMessage(ErrInvalidColor);
        } else {
            visit((StringValue *)0);
        }
    }

    virtual void visit(const AnchorLineValue *)
    {
        if (! (_rhsValue->asAnchorLineValue() || _rhsValue->asUnknownValue()))
            setMessage(ErrAnchorLineExpected);
    }

    Document::Ptr _doc;
    Message _message;
    SourceLocation _location;
    const Value *_rhsValue;
    ExpressionNode *_ast;
};

class ReachesEndCheck : protected Visitor
{
public:
    bool operator()(Node *node)
    {
        _labels.clear();
        _labelledBreaks.clear();
        return check(node) == ReachesEnd;
    }

protected:
    // Sorted by how much code will be reachable from that state, i.e.
    // ReachesEnd is guaranteed to reach more code than Break and so on.
    enum State
    {
        ReachesEnd = 0,
        Break = 1,
        Continue = 2,
        ReturnOrThrow = 3
    };
    State _state;
    QHash<QString, Node *> _labels;
    QSet<Node *> _labelledBreaks;

    virtual void onUnreachable(Node *)
    {}

    virtual State check(Node *node)
    {
        _state = ReachesEnd;
        Node::accept(node, this);
        return _state;
    }

    virtual bool preVisit(Node *ast)
    {
        if (ast->expressionCast())
            return false;
        if (_state == ReachesEnd)
            return true;
        if (Statement *stmt = ast->statementCast())
            onUnreachable(stmt);
        if (FunctionSourceElement *fun = cast<FunctionSourceElement *>(ast))
            onUnreachable(fun->declaration);
        if (StatementSourceElement *stmt = cast<StatementSourceElement *>(ast))
            onUnreachable(stmt->statement);
        return false;
    }

    virtual bool visit(LabelledStatement *ast)
    {
        // get the target statement
        Statement *end = ast->statement;
        forever {
            if (LabelledStatement *label = cast<LabelledStatement *>(end))
                end = label->statement;
            else
                break;
        }
        if (!ast->label.isEmpty())
            _labels[ast->label.toString()] = end;
        return true;
    }

    virtual bool visit(BreakStatement *ast)
    {
        _state = Break;
        if (!ast->label.isEmpty()) {
            if (Node *target = _labels.value(ast->label.toString())) {
                _labelledBreaks.insert(target);
                _state = ReturnOrThrow; // unwind until label is hit
            }
        }
        return false;
    }

    // labelled continues don't change control flow...
    virtual bool visit(ContinueStatement *) { _state = Continue; return false; }

    virtual bool visit(ReturnStatement *) { _state = ReturnOrThrow; return false; }
    virtual bool visit(ThrowStatement *) { _state = ReturnOrThrow; return false; }

    virtual bool visit(IfStatement *ast)
    {
        State ok = check(ast->ok);
        State ko = check(ast->ko);
        _state = qMin(ok, ko);
        return false;
    }

    void handleClause(StatementList *statements, State *result, bool *fallthrough)
    {
        State clauseResult = check(statements);
        if (clauseResult == ReachesEnd) {
            *fallthrough = true;
        } else {
            *fallthrough = false;
            *result = qMin(*result, clauseResult);
        }
    }

    virtual bool visit(SwitchStatement *ast)
    {
        if (!ast->block)
            return false;
        State result = ReturnOrThrow;
        bool lastWasFallthrough = false;

        for (CaseClauses *it = ast->block->clauses; it; it = it->next) {
            if (it->clause)
                handleClause(it->clause->statements, &result, &lastWasFallthrough);
        }
        if (ast->block->defaultClause)
            handleClause(ast->block->defaultClause->statements, &result, &lastWasFallthrough);
        for (CaseClauses *it = ast->block->moreClauses; it; it = it->next) {
            if (it->clause)
                handleClause(it->clause->statements, &result, &lastWasFallthrough);
        }

        if (lastWasFallthrough || !ast->block->defaultClause)
            result = ReachesEnd;
        if (result == Break || _labelledBreaks.contains(ast))
            result = ReachesEnd;
        _state = result;
        return false;
    }

    virtual bool visit(TryStatement *ast)
    {
        State tryBody = check(ast->statement);
        State catchBody = ReturnOrThrow;
        if (ast->catchExpression)
            catchBody = check(ast->catchExpression->statement);
        State finallyBody = ReachesEnd;
        if (ast->finallyExpression)
            finallyBody = check(ast->finallyExpression->statement);

        _state = qMax(qMin(tryBody, catchBody), finallyBody);
        return false;
    }

    bool preconditionLoopStatement(Node *, Statement *body)
    {
        check(body);
        _state = ReachesEnd; // condition could be false...
        return false;
    }

    virtual bool visit(WhileStatement *ast) { return preconditionLoopStatement(ast, ast->statement); }
    virtual bool visit(ForStatement *ast) { return preconditionLoopStatement(ast, ast->statement); }
    virtual bool visit(ForEachStatement *ast) { return preconditionLoopStatement(ast, ast->statement); }
    virtual bool visit(LocalForStatement *ast) { return preconditionLoopStatement(ast, ast->statement); }
    virtual bool visit(LocalForEachStatement *ast) { return preconditionLoopStatement(ast, ast->statement); }

    virtual bool visit(DoWhileStatement *ast)
    {
        check(ast->statement);
        // not necessarily an infinite loop due to labelled breaks
        if (_state == Continue)
            _state = ReturnOrThrow;
        if (_state == Break || _labelledBreaks.contains(ast))
            _state = ReachesEnd;
        return false;
    }
};

class MarkUnreachableCode : protected ReachesEndCheck
{
    QList<Message> _messages;
    bool _emittedWarning;

public:
    QList<Message> operator()(Node *ast)
    {
        _messages.clear();
        check(ast);
        return _messages;
    }

protected:
    virtual State check(Node *node)
    {
        bool oldwarning = _emittedWarning;
        _emittedWarning = false;
        State s = ReachesEndCheck::check(node);
        _emittedWarning = oldwarning;
        return s;
    }

    virtual void onUnreachable(Node *node)
    {
        if (_emittedWarning)
            return;
        _emittedWarning = true;

        Message message(WarnUnreachable, SourceLocation());
        if (Statement *statement = node->statementCast())
            message.location = locationFromRange(statement->firstSourceLocation(), statement->lastSourceLocation());
        else if (ExpressionNode *expr = node->expressionCast())
            message.location = locationFromRange(expr->firstSourceLocation(), expr->lastSourceLocation());
        if (message.isValid())
            _messages += message;
    }
};

class DeclarationsCheck : protected Visitor
{
public:
    QList<Message> operator()(FunctionExpression *function)
    {
        clear();
        for (FormalParameterList *plist = function->formals; plist; plist = plist->next) {
            if (!plist->name.isEmpty())
                _formalParameterNames += plist->name.toString();
        }

        Node::accept(function->body, this);
        return _messages;
    }

    QList<Message> operator()(Node *node)
    {
        clear();
        Node::accept(node, this);
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
        if (ast->name.isEmpty())
            return false;
        const QString &name = ast->name.toString();
        if (!_declaredFunctions.contains(name) && !_declaredVariables.contains(name))
            _possiblyUndeclaredUses[name].append(ast->identifierToken);
        return false;
    }

    bool visit(VariableStatement *ast)
    {
        if (_seenNonDeclarationStatement)
            addMessage(HintDeclarationsShouldBeAtStartOfFunction, ast->declarationKindToken);
        return true;
    }

    bool visit(VariableDeclaration *ast)
    {
        if (ast->name.isEmpty())
            return true;
        const QString &name = ast->name.toString();

        if (_formalParameterNames.contains(name))
            addMessage(WarnAlreadyFormalParameter, ast->identifierToken, name);
        else if (_declaredFunctions.contains(name))
            addMessage(WarnAlreadyFunction, ast->identifierToken, name);
        else if (_declaredVariables.contains(name))
            addMessage(WarnDuplicateDeclaration, ast->identifierToken, name);

        if (_possiblyUndeclaredUses.contains(name)) {
            foreach (const SourceLocation &loc, _possiblyUndeclaredUses.value(name)) {
                addMessage(WarnVarUsedBeforeDeclaration, loc, name);
            }
            _possiblyUndeclaredUses.remove(name);
        }
        _declaredVariables[name] = ast;

        return true;
    }

    bool visit(FunctionDeclaration *ast)
    {
        if (_seenNonDeclarationStatement)
            addMessage(HintDeclarationsShouldBeAtStartOfFunction, ast->functionToken);

        return visit(static_cast<FunctionExpression *>(ast));
    }

    bool visit(FunctionExpression *ast)
    {
        if (ast->name.isEmpty())
            return false;
        const QString &name = ast->name.toString();

        if (_formalParameterNames.contains(name))
            addMessage(WarnAlreadyFormalParameter, ast->identifierToken, name);
        else if (_declaredVariables.contains(name))
            addMessage(WarnAlreadyVar, ast->identifierToken, name);
        else if (_declaredFunctions.contains(name))
            addMessage(WarnDuplicateDeclaration, ast->identifierToken, name);

        if (FunctionDeclaration *decl = cast<FunctionDeclaration *>(ast)) {
            if (_possiblyUndeclaredUses.contains(name)) {
                foreach (const SourceLocation &loc, _possiblyUndeclaredUses.value(name)) {
                    addMessage(WarnFunctionUsedBeforeDeclaration, loc, name);
                }
                _possiblyUndeclaredUses.remove(name);
            }
            _declaredFunctions[name] = decl;
        }

        return false;
    }

private:
    void addMessage(Type type, const SourceLocation &loc, const QString &arg1 = QString())
    {
        _messages.append(Message(type, loc, arg1));
    }

    QList<Message> _messages;
    QStringList _formalParameterNames;
    QHash<QString, VariableDeclaration *> _declaredVariables;
    QHash<QString, FunctionDeclaration *> _declaredFunctions;
    QHash<QString, QList<SourceLocation> > _possiblyUndeclaredUses;
    bool _seenNonDeclarationStatement;
};

class VisualAspectsPropertyBlackList : public QStringList
{
public:
   VisualAspectsPropertyBlackList()
   {
       (*this) << QLatin1String("x") << QLatin1String("y") << QLatin1String("z")
            << QLatin1String("width") << QLatin1String("height") << QLatin1String("color")
            << QLatin1String("opacity") << QLatin1String("scale")
            << QLatin1String("rotation") << QLatin1String("margins")
            << QLatin1String("verticalCenterOffset") << QLatin1String("horizontalCenterOffset")
            << QLatin1String("baselineOffset") << QLatin1String("bottomMargin")
            << QLatin1String("topMargin") << QLatin1String("leftMargin")
            << QLatin1String("rightMargin") << QLatin1String("baseline")
            << QLatin1String("centerIn") << QLatin1String("fill")
            << QLatin1String("left") << QLatin1String("right")
            << QLatin1String("mirrored") << QLatin1String("verticalCenter")
            << QLatin1String("horizontalCenter");

   }
};

class UnsupportedTypesByVisualDesigner : public QStringList
{
public:
    UnsupportedTypesByVisualDesigner()
    {
        (*this) << QLatin1String("Transform") << QLatin1String("Timer")
            << QLatin1String("Rotation") << QLatin1String("Scale")
            << QLatin1String("Translate") << QLatin1String("Package")
            << QLatin1String("Particles");
    }

};
} // end of anonymous namespace

Q_GLOBAL_STATIC(VisualAspectsPropertyBlackList, visualAspectsPropertyBlackList)
Q_GLOBAL_STATIC(UnsupportedTypesByVisualDesigner, unsupportedTypesByVisualDesigner)

Check::Check(Document::Ptr doc, const ContextPtr &context)
    : _doc(doc)
    , _context(context)
    , _scopeChain(doc, _context)
    , _scopeBuilder(&_scopeChain)
    , _importsOk(false)
    , _inStatementBinding(false)
{
    const Imports *imports = context->imports(doc.data());
    if (imports && !imports->importFailed())
        _importsOk = true;

    _enabledMessages = Message::allMessageTypes().toSet();
    disableMessage(HintAnonymousFunctionSpacing);
    disableMessage(HintDeclareVarsInOneLine);
    disableMessage(HintDeclarationsShouldBeAtStartOfFunction);
    disableMessage(HintBinaryOperatorSpacing);
    disableMessage(HintOneStatementPerLine);
    disableMessage(HintExtraParentheses);
    disableMessage(WarnImperativeCodeNotEditableInVisualDesigner);
    disableMessage(WarnUnsupportedTypeInVisualDesigner);
    disableMessage(WarnReferenceToParentItemNotSupportedByVisualDesigner);
    disableMessage(WarnUndefinedValueForVisualDesigner);
    disableMessage(WarnStatesOnlyInRootItemForVisualDesigner);
}

Check::~Check()
{
}

QList<Message> Check::operator()()
{
    _messages.clear();
    scanCommentsForAnnotations();

    Node::accept(_doc->ast(), this);
    warnAboutUnnecessarySuppressions();

    return _messages;
}

void Check::enableMessage(Type type)
{
    _enabledMessages.insert(type);
}

void Check::disableMessage(Type type)
{
    _enabledMessages.remove(type);
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
    QString typeName;
    m_propertyStack.push(StringSet());
    UiQualifiedId *qualifiedTypeId = qualifiedTypeNameId(parent());
    if (qualifiedTypeId) {
        typeName = qualifiedTypeId->name.toString();
        if (typeName == QLatin1String("Component"))
            m_idStack.push(StringSet());
    }

    m_typeStack.push(typeName);

    if (m_idStack.isEmpty())
        m_idStack.push(StringSet());

    return true;
}

void Check::endVisit(UiObjectInitializer *)
{
    m_propertyStack.pop();
    m_typeStack.pop();
    UiObjectDefinition *objectDenition = cast<UiObjectDefinition *>(parent());
    if (objectDenition && objectDenition->qualifiedTypeNameId->name == QLatin1String("Component"))
        m_idStack.pop();
    UiObjectBinding *objectBinding = cast<UiObjectBinding *>(parent());
    if (objectBinding && objectBinding->qualifiedTypeNameId->name == QLatin1String("Component"))
        m_idStack.pop();
}

void Check::checkProperty(UiQualifiedId *qualifiedId)
{
    const QString id = toString(qualifiedId);
    if (id.at(0).isLower()) {
        if (m_propertyStack.top().contains(id))
            addMessage(ErrPropertiesCanOnlyHaveOneBinding, fullLocationForQualifiedId(qualifiedId));
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

static bool expressionAffectsVisualAspects(BinaryExpression *expression)
{
    if (expression->op == QSOperator::Assign
            || expression->op == QSOperator::InplaceSub
            || expression->op == QSOperator::InplaceAdd
            || expression->op == QSOperator::InplaceDiv
            || expression->op == QSOperator::InplaceMul
            || expression->op == QSOperator::InplaceOr
            || expression->op == QSOperator::InplaceXor
            || expression->op == QSOperator::InplaceAnd) {

        const ExpressionNode *lhsValue = expression->left;

        if (const IdentifierExpression* identifierExpression = cast<const IdentifierExpression *>(lhsValue)) {
            if (visualAspectsPropertyBlackList()->contains(identifierExpression->name.toString()))
                return true;
        } else if (const FieldMemberExpression* fieldMemberExpression = cast<const FieldMemberExpression *>(lhsValue)) {
            if (visualAspectsPropertyBlackList()->contains(fieldMemberExpression->name.toString()))
                return true;
        }
    }
    return false;
}

static UiQualifiedId *getRightMostIdentifier(UiQualifiedId *typeId)
{
        if (typeId->next)
            return getRightMostIdentifier(typeId->next);

        return typeId;
}

static bool checkTypeForDesignerSupport(UiQualifiedId *typeId)
{
    return unsupportedTypesByVisualDesigner()->contains(getRightMostIdentifier(typeId)->name.toString());
}

static bool checkTopLevelBindingForParentReference(ExpressionStatement *expStmt, const QString &source)
{
    if (!expStmt)
        return false;

    SourceLocation location = locationFromRange(expStmt->firstSourceLocation(), expStmt->lastSourceLocation());
    QString stmtSource = source.mid(location.begin(), location.length);

    if (stmtSource.contains(QRegExp(QLatin1String("(^|\\W)parent\\."))))
        return true;

    return false;
}

void Check::visitQmlObject(Node *ast, UiQualifiedId *typeId,
                           UiObjectInitializer *initializer)
{
    // Don't do type checks if it's a grouped property binding.
    // For instance: anchors { ... }
    if (_doc->bind()->isGroupedPropertyBinding(ast)) {
        checkScopeObjectMember(typeId);
        // ### don't give up!
        return;
    }

    const SourceLocation typeErrorLocation = fullLocationForQualifiedId(typeId);

    if (checkTypeForDesignerSupport(typeId))
        addMessage(WarnUnsupportedTypeInVisualDesigner, typeErrorLocation);

    if (m_typeStack.count() > 1 && getRightMostIdentifier(typeId)->name.toString() == QLatin1String("State"))
        addMessage(WarnStatesOnlyInRootItemForVisualDesigner, typeErrorLocation);

    bool typeError = false;
    if (_importsOk) {
        const ObjectValue *prototype = _context->lookupType(_doc.data(), typeId);
        if (!prototype) {
            typeError = true;
            addMessage(ErrUnknownComponent, typeErrorLocation);
        } else {
            PrototypeIterator iter(prototype, _context);
            QList<const ObjectValue *> prototypes = iter.all();
            if (iter.error() != PrototypeIterator::NoError)
                typeError = true;
            const ObjectValue *lastPrototype = prototypes.last();
            if (iter.error() == PrototypeIterator::ReferenceResolutionError) {
                if (const QmlPrototypeReference *ref =
                        value_cast<QmlPrototypeReference>(lastPrototype->prototype())) {
                    addMessage(ErrCouldNotResolvePrototypeOf, typeErrorLocation,
                               toString(ref->qmlTypeName()), lastPrototype->className());
                } else {
                    addMessage(ErrCouldNotResolvePrototype, typeErrorLocation,
                               lastPrototype->className());
                }
            } else if (iter.error() == PrototypeIterator::CycleError) {
                addMessage(ErrPrototypeCycle, typeErrorLocation,
                           lastPrototype->className());
            }
        }
    }

    _scopeBuilder.push(ast);

    if (typeError) {
        // suppress subsequent errors about scope object lookup by clearing
        // the scope object list
        // ### todo: better way?
        _scopeChain.setQmlScopeObjects(QList<const ObjectValue *>());
    }

    Node::accept(initializer, this);

    _scopeBuilder.pop();
}

bool Check::visit(UiScriptBinding *ast)
{
    // special case for id property
    if (ast->qualifiedId->name == QLatin1String("id") && ! ast->qualifiedId->next) {
        if (! ast->statement)
            return false;

        const SourceLocation loc = locationFromRange(ast->statement->firstSourceLocation(),
                                                     ast->statement->lastSourceLocation());

        ExpressionStatement *expStmt = cast<ExpressionStatement *>(ast->statement);
        if (!expStmt) {
            addMessage(ErrIdExpected, loc);
            return false;
        }

        QString id;
        if (IdentifierExpression *idExp = cast<IdentifierExpression *>(expStmt->expression)) {
            id = idExp->name.toString();
        } else if (StringLiteral *strExp = cast<StringLiteral *>(expStmt->expression)) {
            id = strExp->value.toString();
            addMessage(ErrInvalidId, loc);
        } else {
            addMessage(ErrIdExpected, loc);
            return false;
        }

        if (id.isEmpty() || (!id.at(0).isLower() && id.at(0) != QLatin1Char('_'))) {
            addMessage(ErrInvalidId, loc);
            return false;
        }

        if (m_idStack.top().contains(id)) {
            addMessage(ErrDuplicateId, loc);
            return false;
        }
        m_idStack.top().insert(id);
    }

    if (m_typeStack.count() == 1
            && visualAspectsPropertyBlackList()->contains(ast->qualifiedId->name.toString())
            && checkTopLevelBindingForParentReference(cast<ExpressionStatement *>(ast->statement), _doc->source())) {
        addMessage(WarnReferenceToParentItemNotSupportedByVisualDesigner,
                   locationFromRange(ast->firstSourceLocation(), ast->lastSourceLocation()));
    }

    checkProperty(ast->qualifiedId);

    if (!ast->statement)
        return false;

    const Value *lhsValue = checkScopeObjectMember(ast->qualifiedId);
    if (lhsValue) {
        Evaluate evaluator(&_scopeChain);
        const Value *rhsValue = evaluator(ast->statement);

        if (visualAspectsPropertyBlackList()->contains(ast->qualifiedId->name.toString()) &&
                rhsValue->asUndefinedValue()) {
            addMessage(WarnUndefinedValueForVisualDesigner,
                       locationFromRange(ast->firstSourceLocation(), ast->lastSourceLocation()));
        }

        const SourceLocation loc = locationFromRange(ast->statement->firstSourceLocation(),
                                                     ast->statement->lastSourceLocation());
        AssignmentCheck assignmentCheck;
        Message message = assignmentCheck(_doc, loc, lhsValue, rhsValue, ast->statement);
        if (message.isValid())
            addMessage(message);
    }

    checkBindingRhs(ast->statement);

    Node::accept(ast->qualifiedId, this);
    _scopeBuilder.push(ast);
    _inStatementBinding = true;
    Node::accept(ast->statement, this);
    _inStatementBinding = false;
    _scopeBuilder.pop();

    return false;
}

bool Check::visit(UiArrayBinding *ast)
{
    checkScopeObjectMember(ast->qualifiedId);
    checkProperty(ast->qualifiedId);

    return true;
}

bool Check::visit(UiPublicMember *ast)
{
    if (ast->type == UiPublicMember::Property) {
        // check if the member type is valid
        if (!ast->memberType.isEmpty()) {
            const QString &name = ast->memberType.toString();
            if (!name.isEmpty() && name.at(0).isLower()) {
                if (!isValidBuiltinPropertyType(name))
                    addMessage(ErrInvalidPropertyType, ast->typeToken, name);
            }

            // warn about dubious use of var/variant
            if (name == QLatin1String("variant") || name == QLatin1String("var")) {
                Evaluate evaluator(&_scopeChain);
                const Value *init = evaluator(ast->statement);
                QString preferedType;
                if (init->asNumberValue())
                    preferedType = tr("'int' or 'real'");
                else if (init->asStringValue())
                    preferedType = QLatin1String("'string'");
                else if (init->asBooleanValue())
                    preferedType = QLatin1String("'bool'");
                else if (init->asColorValue())
                    preferedType = QLatin1String("'color'");
                else if (init == _context->valueOwner()->qmlPointObject())
                    preferedType = QLatin1String("'point'");
                else if (init == _context->valueOwner()->qmlRectObject())
                    preferedType = QLatin1String("'rect'");
                else if (init == _context->valueOwner()->qmlSizeObject())
                    preferedType = QLatin1String("'size'");
                else if (init == _context->valueOwner()->qmlVector3DObject())
                    preferedType = QLatin1String("'vector3d'");

                if (!preferedType.isEmpty())
                    addMessage(HintPreferNonVarPropertyType, ast->typeToken, preferedType);
            }
        }

        checkBindingRhs(ast->statement);

        _scopeBuilder.push(ast);
        _inStatementBinding = true;
        Node::accept(ast->statement, this);
        _inStatementBinding = false;
        Node::accept(ast->binding, this);
        _scopeBuilder.pop();
    }

    return false;
}

bool Check::visit(IdentifierExpression *)
{
    // currently disabled: too many false negatives
    return true;

//    _lastValue = 0;
//    if (!ast->name.isEmpty()) {
//        Evaluate evaluator(&_scopeChain);
//        _lastValue = evaluator.reference(ast);
//        if (!_lastValue)
//            addMessage(ErrUnknownIdentifier, ast->identifierToken);
//        if (const Reference *ref = value_cast<Reference>(_lastValue)) {
//            _lastValue = _context->lookupReference(ref);
//            if (!_lastValue)
//                error(ast->identifierToken, tr("could not resolve"));
//        }
//    }
//    return false;
}

bool Check::visit(FieldMemberExpression *)
{
    // currently disabled: too many false negatives
    return true;

//    Node::accept(ast->base, this);
//    if (!_lastValue)
//        return false;
//    const ObjectValue *obj = _lastValue->asObjectValue();
//    if (!obj) {
//        error(locationFromRange(ast->base->firstSourceLocation(), ast->base->lastSourceLocation()),
//              tr("does not have members"));
//    }
//    if (!obj || ast->name.isEmpty()) {
//        _lastValue = 0;
//        return false;
//    }
//    _lastValue = obj->lookupMember(ast->name.toString(), _context);
//    if (!_lastValue)
//        error(ast->identifierToken, tr("unknown member"));
//    return false;
}

bool Check::visit(FunctionDeclaration *ast)
{
    return visit(static_cast<FunctionExpression *>(ast));
}

bool Check::visit(FunctionExpression *ast)
{
    if (ast->name.isEmpty()) {
        SourceLocation locfunc = ast->functionToken;
        SourceLocation loclparen = ast->lparenToken;
        if (locfunc.isValid() && loclparen.isValid()
                && (locfunc.startLine != loclparen.startLine
                    || locfunc.end() + 1 != loclparen.begin())) {
            addMessage(HintAnonymousFunctionSpacing, locationFromRange(locfunc, loclparen));
        }
    }

    DeclarationsCheck bodyCheck;
    addMessages(bodyCheck(ast));

    MarkUnreachableCode unreachableCheck;
    addMessages(unreachableCheck(ast->body));

    Node::accept(ast->formals, this);

    const bool wasInStatementBinding = _inStatementBinding;
    _inStatementBinding = false;
    _scopeBuilder.push(ast);
    Node::accept(ast->body, this);
    _scopeBuilder.pop();
    _inStatementBinding = wasInStatementBinding;

    return false;
}

static bool shouldAvoidNonStrictEqualityCheck(const Value *lhs, const Value *rhs)
{
    if (lhs->asUnknownValue() || rhs->asUnknownValue())
        return true; // may coerce or not

    if (lhs->asStringValue() && rhs->asNumberValue())
        return true; // coerces string to number

    if (lhs->asObjectValue() && rhs->asNumberValue())
        return true; // coerces object to primitive

    if (lhs->asObjectValue() && rhs->asStringValue())
        return true; // coerces object to primitive

    if (lhs->asBooleanValue() && (!rhs->asBooleanValue()
                                  && !rhs->asUndefinedValue()))
        return true; // coerces bool to number

    return false;
}

bool Check::visit(BinaryExpression *ast)
{
    const QString source = _doc->source();

    // check spacing
    SourceLocation op = ast->operatorToken;
    if ((op.begin() > 0 && !source.at(op.begin() - 1).isSpace())
            || (int(op.end()) < source.size() && !source.at(op.end()).isSpace())) {
        addMessage(HintBinaryOperatorSpacing, op);
    }

    SourceLocation expressionSourceLocation = locationFromRange(ast->firstSourceLocation(),
                                                                ast->lastSourceLocation());
    if (expressionAffectsVisualAspects(ast))
        addMessage(WarnImperativeCodeNotEditableInVisualDesigner, expressionSourceLocation);

    // check ==, !=
    if (ast->op == QSOperator::Equal || ast->op == QSOperator::NotEqual) {
        Evaluate eval(&_scopeChain);
        const Value *lhsValue = eval(ast->left);
        const Value *rhsValue = eval(ast->right);
        if (shouldAvoidNonStrictEqualityCheck(lhsValue, rhsValue)
                || shouldAvoidNonStrictEqualityCheck(rhsValue, lhsValue)) {
            addMessage(MaybeWarnEqualityTypeCoercion, ast->operatorToken);
        }
    }

    // check odd + ++ combinations
    const QLatin1Char newline('\n');
    if (ast->op == QSOperator::Add || ast->op == QSOperator::Sub) {
        QChar match;
        Type msg;
        if (ast->op == QSOperator::Add) {
            match = QLatin1Char('+');
            msg = WarnConfusingPluses;
        } else {
            QTC_CHECK(ast->op == QSOperator::Sub);
            match = QLatin1Char('-');
            msg = WarnConfusingMinuses;
        }

        if (int(op.end()) + 1 < source.size()) {
            const QChar next = source.at(op.end());
            if (next.isSpace() && next != newline
                    && source.at(op.end() + 1) == match)
                addMessage(msg, SourceLocation(op.begin(), 3, op.startLine, op.startColumn));
        }
        if (op.begin() >= 2) {
            const QChar prev = source.at(op.begin() - 1);
            if (prev.isSpace() && prev != newline
                    && source.at(op.begin() - 2) == match)
                addMessage(msg, SourceLocation(op.begin() - 2, 3, op.startLine, op.startColumn - 2));
        }
    }

    return true;
}

bool Check::visit(Block *ast)
{
    if (Node *p = parent()) {
        if (!cast<UiScriptBinding *>(p)
                && !cast<UiPublicMember *>(p)
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
            addMessage(WarnBlock, ast->lbraceToken);
        }
        if (!ast->statements
                && cast<UiPublicMember *>(p)
                && ast->lbraceToken.startLine == ast->rbraceToken.startLine) {
            addMessage(WarnUnintentinalEmptyBlock, locationFromRange(ast->firstSourceLocation(), ast->lastSourceLocation()));
        }
    }
    return true;
}

bool Check::visit(WithStatement *ast)
{
    addMessage(WarnWith, ast->withToken);
    return true;
}

bool Check::visit(VoidExpression *ast)
{
    addMessage(WarnVoid, ast->voidToken);
    return true;
}

bool Check::visit(Expression *ast)
{
    if (ast->left && ast->right) {
        Node *p = parent();
        if (!cast<ForStatement *>(p)
                && !cast<LocalForStatement *>(p)) {
            addMessage(WarnComma, ast->commaToken);
        }
    }
    return true;
}

bool Check::visit(ExpressionStatement *ast)
{
    if (ast->expression) {
        bool ok = cast<CallExpression *>(ast->expression)
                || cast<DeleteExpression *>(ast->expression)
                || cast<PreDecrementExpression *>(ast->expression)
                || cast<PreIncrementExpression *>(ast->expression)
                || cast<PostIncrementExpression *>(ast->expression)
                || cast<PostDecrementExpression *>(ast->expression)
                || cast<FunctionExpression *>(ast->expression);
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
        if (!ok)
            ok = _inStatementBinding;

        if (!ok) {
            addMessage(WarnConfusingExpressionStatement,
                       locationFromRange(ast->firstSourceLocation(), ast->lastSourceLocation()));
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

bool Check::visit(CaseBlock *ast)
{
    QList< QPair<SourceLocation, StatementList *> > clauses;
    for (CaseClauses *it = ast->clauses; it; it = it->next)
        clauses += qMakePair(it->clause->caseToken, it->clause->statements);
    if (ast->defaultClause)
        clauses += qMakePair(ast->defaultClause->defaultToken, ast->defaultClause->statements);
    for (CaseClauses *it = ast->moreClauses; it; it = it->next)
        clauses += qMakePair(it->clause->caseToken, it->clause->statements);

    // check all but the last clause for fallthrough
    for (int i = 0; i < clauses.size() - 1; ++i) {
        const SourceLocation nextToken = clauses[i + 1].first;
        checkCaseFallthrough(clauses[i].second, clauses[i].first, nextToken);
    }
    return true;
}

static QString functionName(ExpressionNode *ast, SourceLocation *location)
{
    if (IdentifierExpression *id = cast<IdentifierExpression *>(ast)) {
        if (!id->name.isEmpty()) {
            *location = id->identifierToken;
            return id->name.toString();
        }
    } else if (FieldMemberExpression *fme = cast<FieldMemberExpression *>(ast)) {
        if (!fme->name.isEmpty()) {
            *location = fme->identifierToken;
            return fme->name.toString();
        }
    }
    return QString();
}

void Check::checkNewExpression(ExpressionNode *ast)
{
    SourceLocation location;
    const QString name = functionName(ast, &location);
    if (name.isEmpty())
        return;
    if (!name.at(0).isUpper())
        addMessage(WarnNewWithLowercaseFunction, location);
}

void Check::checkBindingRhs(Statement *statement)
{
    if (!statement)
        return;

    DeclarationsCheck bodyCheck;
    addMessages(bodyCheck(statement));

    MarkUnreachableCode unreachableCheck;
    addMessages(unreachableCheck(statement));
}

void Check::checkExtraParentheses(ExpressionNode *expression)
{
    if (NestedExpression *nested = cast<NestedExpression *>(expression))
        addMessage(HintExtraParentheses, nested->lparenToken);
}

void Check::addMessages(const QList<Message> &messages)
{
    foreach (const Message &msg, messages)
        addMessage(msg);
}

static bool hasOnlySpaces(const QString &s)
{
    for (int i = 0; i < s.size(); ++i)
        if (!s.at(i).isSpace())
            return false;
    return true;
}

void Check::addMessage(const Message &message)
{
    if (message.isValid() && _enabledMessages.contains(message.type)) {
        if (m_disabledMessageTypesByLine.contains(message.location.startLine)) {
            QList<MessageTypeAndSuppression> &disabledMessages = m_disabledMessageTypesByLine[message.location.startLine];
            for (int i = 0; i < disabledMessages.size(); ++i) {
                if (disabledMessages[i].type == message.type) {
                    disabledMessages[i].wasSuppressed = true;
                    return;
                }
            }
        }

        _messages += message;
    }
}

void Check::addMessage(Type type, const SourceLocation &location, const QString &arg1, const QString &arg2)
{
    addMessage(Message(type, location, arg1, arg2));
}

void Check::scanCommentsForAnnotations()
{
    m_disabledMessageTypesByLine.clear();
    QRegExp disableCommentPattern(Message::suppressionPattern());

    foreach (const SourceLocation &commentLoc, _doc->engine()->comments()) {
        const QString &comment = _doc->source().mid(commentLoc.begin(), commentLoc.length);

        // enable all checks annotation
        if (comment.contains(QLatin1String("@enable-all-checks")))
            _enabledMessages = Message::allMessageTypes().toSet();

        // find all disable annotations
        int lastOffset = -1;
        QList<MessageTypeAndSuppression> disabledMessageTypes;
        forever {
            lastOffset = disableCommentPattern.indexIn(comment, lastOffset + 1);
            if (lastOffset == -1)
                break;
            MessageTypeAndSuppression entry;
            entry.type = static_cast<StaticAnalysis::Type>(disableCommentPattern.cap(1).toInt());
            entry.wasSuppressed = false;
            entry.suppressionSource = SourceLocation(commentLoc.offset + lastOffset,
                                                     disableCommentPattern.matchedLength(),
                                                     commentLoc.startLine,
                                                     commentLoc.startColumn + lastOffset);
            disabledMessageTypes += entry;
        }
        if (!disabledMessageTypes.isEmpty()) {
            int appliesToLine = commentLoc.startLine;

            // if the comment is preceded by spaces only, it applies to the next line
            // note: startColumn is 1-based and *after* the starting // or /*
            if (commentLoc.startColumn >= 3) {
                const QString &beforeComment = _doc->source().mid(commentLoc.begin() - commentLoc.startColumn + 1,
                                                                  commentLoc.startColumn - 3);
                if (hasOnlySpaces(beforeComment))
                    ++appliesToLine;
            }

            m_disabledMessageTypesByLine[appliesToLine] += disabledMessageTypes;
        }
    }
}

void Check::warnAboutUnnecessarySuppressions()
{
    QHashIterator< int, QList<MessageTypeAndSuppression> > it(m_disabledMessageTypesByLine);
    while (it.hasNext()) {
        it.next();
        foreach (const MessageTypeAndSuppression &entry, it.value()) {
            if (!entry.wasSuppressed)
                addMessage(WarnUnnecessaryMessageSuppression, entry.suppressionSource);
        }
    }
}

bool Check::visit(NewExpression *ast)
{
    checkNewExpression(ast->expression);
    return true;
}

bool Check::visit(NewMemberExpression *ast)
{
    checkNewExpression(ast->base);

    // check for Number, Boolean, etc constructor usage
    if (IdentifierExpression *idExp = cast<IdentifierExpression *>(ast->base)) {
        const QStringRef name = idExp->name;
        if (name == QLatin1String("Number")) {
            addMessage(WarnNumberConstructor, idExp->identifierToken);
        } else if (name == QLatin1String("Boolean")) {
            addMessage(WarnBooleanConstructor, idExp->identifierToken);
        } else if (name == QLatin1String("String")) {
            addMessage(WarnStringConstructor, idExp->identifierToken);
        } else if (name == QLatin1String("Object")) {
            addMessage(WarnObjectConstructor, idExp->identifierToken);
        } else if (name == QLatin1String("Array")) {
            bool ok = false;
            if (ast->arguments && ast->arguments->expression && !ast->arguments->next) {
                Evaluate evaluate(&_scopeChain);
                const Value *arg = evaluate(ast->arguments->expression);
                if (arg->asNumberValue() || arg->asUnknownValue())
                    ok = true;
            }
            if (!ok)
                addMessage(WarnArrayConstructor, idExp->identifierToken);
        } else if (name == QLatin1String("Function")) {
            addMessage(WarnFunctionConstructor, idExp->identifierToken);
        }
    }

    return true;
}

bool Check::visit(CallExpression *ast)
{
    // check for capitalized function name being called
    SourceLocation location;
    const QString name = functionName(ast->base, &location);
    if (!name.isEmpty() && name.at(0).isUpper()
            && name != QLatin1String("String")
            && name != QLatin1String("Boolean")
            && name != QLatin1String("Date")
            && name != QLatin1String("Number")
            && name != QLatin1String("Object")) {
        addMessage(WarnExpectedNewWithUppercaseFunction, location);
    }
    if (cast<IdentifierExpression *>(ast->base) && name == QLatin1String("eval"))
        addMessage(WarnEval, location);
    return true;
}

bool Check::visit(StatementList *ast)
{
    SourceLocation warnStart;
    SourceLocation warnEnd;
    unsigned currentLine = 0;
    for (StatementList *it = ast; it; it = it->next) {
        if (!it->statement)
            continue;
        const SourceLocation itLoc = it->statement->firstSourceLocation();
        if (itLoc.startLine != currentLine) { // first statement on a line
            if (warnStart.isValid())
                addMessage(HintOneStatementPerLine, locationFromRange(warnStart, warnEnd));
            warnStart = SourceLocation();
            currentLine = itLoc.startLine;
        } else { // other statements on the same line
            if (!warnStart.isValid())
                warnStart = itLoc;
            warnEnd = it->statement->lastSourceLocation();
        }
    }
    if (warnStart.isValid())
        addMessage(HintOneStatementPerLine, locationFromRange(warnStart, warnEnd));

    return true;
}

bool Check::visit(ReturnStatement *ast)
{
    checkExtraParentheses(ast->expression);
    return true;
}

bool Check::visit(ThrowStatement *ast)
{
    checkExtraParentheses(ast->expression);
    return true;
}

bool Check::visit(DeleteExpression *ast)
{
    checkExtraParentheses(ast->expression);
    return true;
}

bool Check::visit(TypeOfExpression *ast)
{
    checkExtraParentheses(ast->expression);
    return true;
}

/// When something is changed here, also change ReadingContext::lookupProperty in
/// texttomodelmerger.cpp
/// ### Maybe put this into the context as a helper method.
const Value *Check::checkScopeObjectMember(const UiQualifiedId *id)
{
    if (!_importsOk)
        return 0;

    QList<const ObjectValue *> scopeObjects = _scopeChain.qmlScopeObjects();
    if (scopeObjects.isEmpty())
        return 0;

    if (! id)
        return 0; // ### error?

    if (id->name.isEmpty()) // possible after error recovery
        return 0;

    QString propertyName = id->name.toString();

    if (propertyName == QLatin1String("id") && ! id->next)
        return 0; // ### should probably be a special value

    // attached properties
    bool isAttachedProperty = false;
    if (! propertyName.isEmpty() && propertyName[0].isUpper()) {
        isAttachedProperty = true;
        if (const ObjectValue *qmlTypes = _scopeChain.qmlTypes())
            scopeObjects += qmlTypes;
    }

    if (scopeObjects.isEmpty())
        return 0;

    // global lookup for first part of id
    const Value *value = 0;
    for (int i = scopeObjects.size() - 1; i >= 0; --i) {
        value = scopeObjects[i]->lookupMember(propertyName, _context);
        if (value)
            break;
    }
    if (!value) {
        addMessage(ErrInvalidPropertyName, id->identifierToken, propertyName);
        return 0;
    }

    // can't look up members for attached properties
    if (isAttachedProperty)
        return 0;

    // resolve references
    if (const Reference *ref = value->asReference())
        value = _context->lookupReference(ref);

    // member lookup
    const UiQualifiedId *idPart = id;
    while (idPart->next) {
        const ObjectValue *objectValue = value_cast<ObjectValue>(value);
        if (! objectValue) {
            addMessage(ErrDoesNotHaveMembers, idPart->identifierToken, propertyName);
            return 0;
        }

        if (idPart->next->name.isEmpty()) {
            // somebody typed "id." and error recovery still gave us a valid tree,
            // so just bail out here.
            return 0;
        }

        idPart = idPart->next;
        propertyName = idPart->name.toString();

        value = objectValue->lookupMember(propertyName, _context);
        if (! value) {
            addMessage(ErrInvalidMember, idPart->identifierToken, propertyName, objectValue->className());
            return 0;
        }
    }

    return value;
}

void Check::checkAssignInCondition(AST::ExpressionNode *condition)
{
    if (BinaryExpression *binary = cast<BinaryExpression *>(condition)) {
        if (binary->op == QSOperator::Assign)
            addMessage(WarnAssignmentInCondition, binary->operatorToken);
    }
}

void Check::checkCaseFallthrough(StatementList *statements, SourceLocation errorLoc, SourceLocation nextLoc)
{
    if (!statements)
        return;

    ReachesEndCheck check;
    if (check(statements)) {
        // check for fallthrough comments
        if (nextLoc.isValid()) {
            quint32 afterLastStatement = 0;
            for (StatementList *it = statements; it; it = it->next) {
                if (!it->next)
                    afterLastStatement = it->statement->lastSourceLocation().end();
            }

            foreach (const SourceLocation &comment, _doc->engine()->comments()) {
                if (comment.begin() < afterLastStatement
                        || comment.end() > nextLoc.begin())
                    continue;

                const QString &commentText = _doc->source().mid(comment.begin(), comment.length);
                if (commentText.contains(QLatin1String("fall through"))
                        || commentText.contains(QLatin1String("fall-through"))
                        || commentText.contains(QLatin1String("fallthrough"))) {
                    return;
                }
            }
        }

        addMessage(WarnCaseWithoutFlowControl, errorLoc);
    }
}

Node *Check::parent(int distance)
{
    const int index = _chain.size() - 2 - distance;
    if (index < 0)
        return 0;
    return _chain.at(index);
}
