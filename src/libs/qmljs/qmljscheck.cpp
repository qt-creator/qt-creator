/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "qmljscheck.h"
#include "qmljsbind.h"
#include "qmljscontext.h"
#include "qmljsevaluate.h"
#include "parser/qmljsast_p.h"

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtGui/QColor>
#include <QtGui/QApplication>

using namespace QmlJS;
using namespace QmlJS::AST;

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
        if (iter->identifierToken.isValid())
            end = iter->identifierToken;
    }

    return locationFromRange(start, end);
}


DiagnosticMessage QmlJS::errorMessage(const AST::SourceLocation &loc, const QString &message)
{
    return DiagnosticMessage(DiagnosticMessage::Error, loc, message);
}

namespace {
class SharedData
{
public:
    SharedData()
    {
        validBuiltinPropertyNames.insert(QLatin1String("action"));
        validBuiltinPropertyNames.insert(QLatin1String("bool"));
        validBuiltinPropertyNames.insert(QLatin1String("color"));
        validBuiltinPropertyNames.insert(QLatin1String("date"));
        validBuiltinPropertyNames.insert(QLatin1String("double"));
        validBuiltinPropertyNames.insert(QLatin1String("enumeration"));
        validBuiltinPropertyNames.insert(QLatin1String("font"));
        validBuiltinPropertyNames.insert(QLatin1String("int"));
        validBuiltinPropertyNames.insert(QLatin1String("list"));
        validBuiltinPropertyNames.insert(QLatin1String("point"));
        validBuiltinPropertyNames.insert(QLatin1String("real"));
        validBuiltinPropertyNames.insert(QLatin1String("rect"));
        validBuiltinPropertyNames.insert(QLatin1String("size"));
        validBuiltinPropertyNames.insert(QLatin1String("string"));
        validBuiltinPropertyNames.insert(QLatin1String("time"));
        validBuiltinPropertyNames.insert(QLatin1String("url"));
        validBuiltinPropertyNames.insert(QLatin1String("variant"));
        validBuiltinPropertyNames.insert(QLatin1String("vector3d"));
        validBuiltinPropertyNames.insert(QLatin1String("alias"));
    }

    QSet<QString> validBuiltinPropertyNames;
};
} // anonymous namespace
Q_GLOBAL_STATIC(SharedData, sharedData)

bool QmlJS::isValidBuiltinPropertyType(const QString &name)
{
    return sharedData()->validBuiltinPropertyNames.contains(name);
}

namespace {

class AssignmentCheck : public ValueVisitor
{
public:
    DiagnosticMessage operator()(
            const Document::Ptr &document,
            const SourceLocation &location,
            const Value *lhsValue,
            const Value *rhsValue,
            Node *ast)
    {
        _doc = document;
        _message = DiagnosticMessage(DiagnosticMessage::Error, location, QString());
        _rhsValue = rhsValue;
        if (ExpressionStatement *expStmt = cast<ExpressionStatement *>(ast))
            _ast = expStmt->expression;
        else
            _ast = ast->expressionCast();

        if (lhsValue)
            lhsValue->accept(this);

        return _message;
    }

    virtual void visit(const NumberValue *value)
    {
        if (const QmlEnumValue *enumValue = dynamic_cast<const QmlEnumValue *>(value)) {
            if (StringLiteral *stringLiteral = cast<StringLiteral *>(_ast)) {
                const QString valueName = stringLiteral->value.toString();

                if (!enumValue->keys().contains(valueName)) {
                    _message.message = Check::tr("unknown value for enum");
                }
            } else if (! _rhsValue->asStringValue() && ! _rhsValue->asNumberValue()
                       && ! _rhsValue->asUndefinedValue()) {
                _message.message = Check::tr("enum value is not a string or number");
            }
        } else {
            if (cast<TrueLiteral *>(_ast)
                    || cast<FalseLiteral *>(_ast)) {
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
                || cast<TrueLiteral *>(_ast)
                || cast<FalseLiteral *>(_ast)) {
            _message.message = Check::tr("string value expected");
        }

        if (value && value->asUrlValue()) {
            if (StringLiteral *literal = cast<StringLiteral *>(_ast)) {
                QUrl url(literal->value.toString());
                if (!url.isValid() && !url.isEmpty()) {
                    _message.message = Check::tr("not a valid url");
                } else {
                    QString fileName = url.toLocalFile();
                    if (!fileName.isEmpty()) {
                        if (QFileInfo(fileName).isRelative()) {
                            fileName.prepend(QDir::separator());
                            fileName.prepend(_doc->path());
                        }
                        if (!QFileInfo(fileName).exists()) {
                            _message.message = Check::tr("file or directory does not exist");
                            _message.kind = DiagnosticMessage::Warning;
                        }
                    }
                }
            }
        }
    }

    virtual void visit(const ColorValue *)
    {
        if (StringLiteral *stringLiteral = cast<StringLiteral *>(_ast)) {
            if (!toQColor(stringLiteral->value.toString()).isValid())
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
    QList<DiagnosticMessage> _messages;
    bool _emittedWarning;

public:
    QList<DiagnosticMessage> operator()(Node *ast)
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

        DiagnosticMessage message(DiagnosticMessage::Warning, SourceLocation(), Check::tr("unreachable"));
        if (Statement *statement = node->statementCast())
            message.loc = locationFromRange(statement->firstSourceLocation(), statement->lastSourceLocation());
        else if (ExpressionNode *expr = node->expressionCast())
            message.loc = locationFromRange(expr->firstSourceLocation(), expr->lastSourceLocation());
        if (message.loc.isValid())
            _messages += message;
    }
};

class DeclarationsCheck : protected Visitor
{
public:
    QList<DiagnosticMessage> operator()(FunctionExpression *function, Check::Options options)
    {
        clear();
        _options = options;
        for (FormalParameterList *plist = function->formals; plist; plist = plist->next) {
            if (!plist->name.isEmpty())
                _formalParameterNames += plist->name.toString();
        }

        Node::accept(function->body, this);
        return _messages;
    }

    QList<DiagnosticMessage> operator()(Node *node, Check::Options options)
    {
        clear();
        _options = options;
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
        if (_options & Check::WarnDeclarationsNotStartOfFunction && _seenNonDeclarationStatement) {
            warning(ast->declarationKindToken, Check::tr("declarations should be at the start of a function"));
        }
        return true;
    }

    bool visit(VariableDeclaration *ast)
    {
        if (ast->name.isEmpty())
            return true;
        const QString &name = ast->name.toString();

        if (_options & Check::WarnDuplicateDeclaration) {
            if (_formalParameterNames.contains(name)) {
                warning(ast->identifierToken, Check::tr("already a formal parameter"));
            } else if (_declaredFunctions.contains(name)) {
                warning(ast->identifierToken, Check::tr("already declared as function"));
            } else if (_declaredVariables.contains(name)) {
                warning(ast->identifierToken, Check::tr("duplicate declaration"));
            }
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
        if (ast->name.isEmpty())
            return false;
        const QString &name = ast->name.toString();

        if (_options & Check::WarnDuplicateDeclaration) {
            if (_formalParameterNames.contains(name)) {
                warning(ast->identifierToken, Check::tr("already a formal parameter"));
            } else if (_declaredVariables.contains(name)) {
                warning(ast->identifierToken, Check::tr("already declared as var"));
            } else if (_declaredFunctions.contains(name)) {
                warning(ast->identifierToken, Check::tr("duplicate declaration"));
            }
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


Check::Check(Document::Ptr doc, const ContextPtr &context)
    : _doc(doc)
    , _context(context)
    , _scopeChain(doc, _context)
    , _scopeBuilder(&_scopeChain)
    , _options(WarnDangerousNonStrictEqualityChecks | WarnBlocks | WarnWith
               | WarnVoid | WarnCommaExpression | WarnExpressionStatement
               | WarnAssignInCondition | WarnUseBeforeDeclaration | WarnDuplicateDeclaration
               | WarnCaseWithoutFlowControlEnd | WarnNonCapitalizedNew
               | WarnCallsOfCapitalizedFunctions | WarnUnreachablecode
               | ErrCheckTypeErrors)
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
    if (objectDefinition && objectDefinition->qualifiedTypeNameId->name == "Component")
        m_idStack.push(StringSet());
    UiObjectBinding *objectBinding = cast<UiObjectBinding *>(parent());
    if (objectBinding && objectBinding->qualifiedTypeNameId->name == "Component")
        m_idStack.push(StringSet());
    if (m_idStack.isEmpty())
        m_idStack.push(StringSet());
    return true;
}

void Check::endVisit(UiObjectInitializer *)
{
    m_propertyStack.pop();
    UiObjectDefinition *objectDenition = cast<UiObjectDefinition *>(parent());
    if (objectDenition && objectDenition->qualifiedTypeNameId->name == "Component")
        m_idStack.pop();
    UiObjectBinding *objectBinding = cast<UiObjectBinding *>(parent());
    if (objectBinding && objectBinding->qualifiedTypeNameId->name == "Component")
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
    // Don't do type checks if it's a grouped property binding.
    // For instance: anchors { ... }
    if (_doc->bind()->isGroupedPropertyBinding(ast)) {
        checkScopeObjectMember(typeId);
        // ### don't give up!
        return;
    }

    bool typeError = false;
    const SourceLocation typeErrorLocation = fullLocationForQualifiedId(typeId);
    const ObjectValue *prototype = _context->lookupType(_doc.data(), typeId);
    if (!prototype) {
        typeError = true;
        if (_options & ErrCheckTypeErrors)
            error(typeErrorLocation,
                  Check::tr("unknown type"));
    } else {
        PrototypeIterator iter(prototype, _context);
        QList<const ObjectValue *> prototypes = iter.all();
        if (iter.error() != PrototypeIterator::NoError)
            typeError = true;
        if (_options & ErrCheckTypeErrors) {
            const ObjectValue *lastPrototype = prototypes.last();
            if (iter.error() == PrototypeIterator::ReferenceResolutionError) {
                if (const QmlPrototypeReference *ref =
                        dynamic_cast<const QmlPrototypeReference *>(lastPrototype->prototype())) {
                    error(typeErrorLocation,
                          Check::tr("could not resolve the prototype %1 of %2").arg(
                              Bind::toString(ref->qmlTypeName()), lastPrototype->className()));
                } else {
                    error(typeErrorLocation,
                          Check::tr("could not resolve the prototype of %1").arg(
                              lastPrototype->className()));
                }
            } else if (iter.error() == PrototypeIterator::CycleError) {
                error(typeErrorLocation,
                      Check::tr("prototype cycle, the last non-repeated object is %1").arg(
                          lastPrototype->className()));
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
            error(loc, Check::tr("expected id"));
            return false;
        }

        QString id;
        if (IdentifierExpression *idExp = cast<IdentifierExpression *>(expStmt->expression)) {
            id = idExp->name.toString();
        } else if (StringLiteral *strExp = cast<StringLiteral *>(expStmt->expression)) {
            id = strExp->value.toString();
            warning(loc, Check::tr("using string literals for ids is discouraged"));
        } else {
            error(loc, Check::tr("expected id"));
            return false;
        }

        if (id.isEmpty() || (!id.at(0).isLower() && id.at(0) != '_')) {
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

    if (!ast->statement)
        return false;

    const Value *lhsValue = checkScopeObjectMember(ast->qualifiedId);
    if (lhsValue) {
        Evaluate evaluator(&_scopeChain);
        const Value *rhsValue = evaluator(ast->statement);

        const SourceLocation loc = locationFromRange(ast->statement->firstSourceLocation(),
                                                     ast->statement->lastSourceLocation());
        AssignmentCheck assignmentCheck;
        DiagnosticMessage message = assignmentCheck(_doc, loc, lhsValue, rhsValue, ast->statement);
        if (! message.message.isEmpty())
            _messages += message;
    }

    checkBindingRhs(ast->statement);

    Node::accept(ast->qualifiedId, this);
    _scopeBuilder.push(ast);
    Node::accept(ast->statement, this);
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
    // check if the member type is valid
    if (!ast->memberType.isEmpty()) {
        const QString &name = ast->memberType.toString();
        if (!name.isEmpty() && name.at(0).isLower()) {
            if (!isValidBuiltinPropertyType(name))
                error(ast->typeToken, tr("'%1' is not a valid property type").arg(name));
        }
    }

    checkBindingRhs(ast->statement);

    _scopeBuilder.push(ast);
    Node::accept(ast->statement, this);
    Node::accept(ast->binding, this);
    _scopeBuilder.pop();

    return false;
}

bool Check::visit(IdentifierExpression *ast)
{
    // currently disabled: too many false negatives
    return true;

    _lastValue = 0;
    if (!ast->name.isEmpty()) {
        Evaluate evaluator(&_scopeChain);
        _lastValue = evaluator.reference(ast);
        if (!_lastValue)
            error(ast->identifierToken, tr("unknown identifier"));
        if (const Reference *ref = value_cast<const Reference *>(_lastValue)) {
            _lastValue = _context->lookupReference(ref);
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
    if (!obj || ast->name.isEmpty()) {
        _lastValue = 0;
        return false;
    }
    _lastValue = obj->lookupMember(ast->name.toString(), _context);
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
    DeclarationsCheck bodyCheck;
    _messages += bodyCheck(ast, _options);
    if (_options & WarnUnreachablecode) {
        MarkUnreachableCode unreachableCheck;
        _messages += unreachableCheck(ast->body);
    }

    Node::accept(ast->formals, this);
    _scopeBuilder.push(ast);
    Node::accept(ast->body, this);
    _scopeBuilder.pop();
    return false;
}

static bool shouldAvoidNonStrictEqualityCheck(const Value *lhs, const Value *rhs)
{
    // we currently use undefined as a "we don't know" value
    if (lhs->asUndefinedValue() || rhs->asUndefinedValue())
        return true;

    if (lhs->asStringValue() && rhs->asNumberValue())
        return true; // coerces string to number

    if (lhs->asObjectValue() && rhs->asNumberValue())
        return true; // coerces object to primitive

    if (lhs->asObjectValue() && rhs->asStringValue())
        return true; // coerces object to primitive

    if (lhs->asBooleanValue() && !rhs->asBooleanValue())
        return true; // coerces bool to number

    return false;
}

bool Check::visit(BinaryExpression *ast)
{
    if (ast->op == QSOperator::Equal || ast->op == QSOperator::NotEqual) {
        bool warn = _options & WarnAllNonStrictEqualityChecks;
        if (!warn && _options & WarnDangerousNonStrictEqualityChecks) {
            Evaluate eval(&_scopeChain);
            const Value *lhs = eval(ast->left);
            const Value *rhs = eval(ast->right);
            warn = shouldAvoidNonStrictEqualityCheck(lhs, rhs)
                    || shouldAvoidNonStrictEqualityCheck(rhs, lhs);
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
            warning(ast->lbraceToken, tr("blocks do not introduce a new scope, avoid"));
        }
        if (!ast->statements
                && (cast<UiPublicMember *>(p)
                    || cast<UiScriptBinding *>(p))) {
            warning(locationFromRange(ast->firstSourceLocation(), ast->lastSourceLocation()),
                    tr("unintentional empty block, use ({}) for empty object literal"));
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
        if (!ok) {
            for (int i = 0; Node *p = parent(i); ++i) {
                if (UiScriptBinding *binding = cast<UiScriptBinding *>(p)) {
                    if (!cast<Block *>(binding->statement)) {
                        ok = true;
                        break;
                    }
                }
                if (UiPublicMember *member = cast<UiPublicMember *>(p)) {
                    if (!cast<Block *>(member->statement)) {
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
    if (!(_options & WarnNonCapitalizedNew))
        return;
    SourceLocation location;
    const QString name = functionName(ast, &location);
    if (name.isEmpty())
        return;
    if (!name.at(0).isUpper()) {
        warning(location, tr("'new' should only be used with functions that start with an uppercase letter"));
    }
}

void Check::checkBindingRhs(Statement *statement)
{
    if (!statement)
        return;

    DeclarationsCheck bodyCheck;
    _messages += bodyCheck(statement, _options);
    if (_options & WarnUnreachablecode) {
        MarkUnreachableCode unreachableCheck;
        _messages += unreachableCheck(statement);
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
    return true;
}

bool Check::visit(CallExpression *ast)
{
    // check for capitalized function name being called
    if (_options & WarnCallsOfCapitalizedFunctions) {
        SourceLocation location;
        const QString name = functionName(ast->base, &location);
        if (!name.isEmpty() && name.at(0).isUpper()) {
            warning(location, tr("calls of functions that start with an uppercase letter should use 'new'"));
        }
    }
    return true;
}

/// When something is changed here, also change ReadingContext::lookupProperty in
/// texttomodelmerger.cpp
/// ### Maybe put this into the context as a helper method.
const Value *Check::checkScopeObjectMember(const UiQualifiedId *id)
{
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
        error(id->identifierToken,
              Check::tr("'%1' is not a valid property name").arg(propertyName));
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
        const ObjectValue *objectValue = value_cast<const ObjectValue *>(value);
        if (! objectValue) {
            error(idPart->identifierToken,
                  Check::tr("'%1' does not have members").arg(propertyName));
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
    if (!statements || !(_options & WarnCaseWithoutFlowControlEnd))
        return;

    ReachesEndCheck check;
    if (check(statements)) {
        warning(errorLoc, tr("case is not terminated and not empty"));
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
