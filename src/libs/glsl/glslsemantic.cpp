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

#include "glslsemantic.h"
#include "glslengine.h"
#include "glslparser.h"
#include "glslsymbols.h"
#include "glsltypes.h"
#include <QDebug>

using namespace GLSL;

Semantic::Semantic()
    : _engine(0)
    , _scope(0)
    , _type(0)
{
}

Semantic::~Semantic()
{
}

Engine *Semantic::switchEngine(Engine *engine)
{
    Engine *previousEngine = _engine;
    _engine = engine;
    return previousEngine;
}

Scope *Semantic::switchScope(Scope *scope)
{
    Scope *previousScope = _scope;
    _scope = scope;
    return previousScope;
}

Semantic::ExprResult Semantic::expression(ExpressionAST *ast)
{
    Semantic::ExprResult r(_engine->undefinedType());
    std::swap(_expr, r);
    accept(ast);
    std::swap(_expr, r);
    return r;
}

void Semantic::statement(StatementAST *ast)
{
    accept(ast);
}

const Type *Semantic::type(TypeAST *ast)
{
    const Type *t = _engine->undefinedType();
    std::swap(_type, t);
    accept(ast);
    std::swap(_type, t);
    return t;
}

void Semantic::declaration(DeclarationAST *ast)
{
    accept(ast);
}

void Semantic::translationUnit(TranslationUnitAST *ast, Scope *globalScope, Engine *engine)
{
    Engine *previousEngine = switchEngine(engine);
    Scope *previousScope = switchScope(globalScope);
    if (ast) {
        for (List<DeclarationAST *> *it = ast->declarations; it; it = it->next) {
            DeclarationAST *decl = it->value;
            declaration(decl);
        }
    }
    (void) switchScope(previousScope);
    (void) switchEngine(previousEngine);
}

Semantic::ExprResult Semantic::expression(ExpressionAST *ast, Scope *scope, Engine *engine)
{
    ExprResult result(engine->undefinedType());
    if (ast && scope) {
        Engine *previousEngine = switchEngine(engine);
        Scope *previousScope = switchScope(scope);
        result = expression(ast);
        (void) switchScope(previousScope);
        (void) switchEngine(previousEngine);
    }
    return result;
}

Semantic::ExprResult Semantic::functionIdentifier(FunctionIdentifierAST *ast)
{
    ExprResult result;
    if (ast) {
        if (ast->name) {
            if (Symbol *s = _scope->lookup(*ast->name)) {
                if (s->asOverloadSet() != 0 || s->asFunction() != 0)
                    result.type = s->type();
                else
                    _engine->error(ast->lineno, QString::fromLatin1("`%1' cannot be used as a function").arg(*ast->name));
            } else {
                _engine->error(ast->lineno, QString::fromLatin1("`%1' was not declared in this scope").arg(*ast->name));
            }
        } else if (ast->type) {
            const Type *ty = type(ast->type);
            result.type = ty;
        }
    }

    return result;
}

Symbol *Semantic::field(StructTypeAST::Field *ast)
{
    // ast->name
    const Type *ty = type(ast->type);
    QString name;
    if (ast->name)
        name = *ast->name;
    return _engine->newVariable(_scope, name, ty);
}

void Semantic::parameterDeclaration(ParameterDeclarationAST *ast, Function *fun)
{
    const Type *ty = type(ast->type);
    QString name;
    if (ast->name)
        name = *ast->name;
    Argument *arg = _engine->newArgument(fun, name, ty);
    fun->addArgument(arg);
}

bool Semantic::visit(TranslationUnitAST *ast)
{
    Q_UNUSED(ast);
    Q_ASSERT(!"unreachable");
    return false;
}

bool Semantic::visit(FunctionIdentifierAST *ast)
{
    Q_UNUSED(ast);
    Q_ASSERT(!"unreachable");
    return false;
}

bool Semantic::visit(StructTypeAST::Field *ast)
{
    Q_UNUSED(ast);
    Q_ASSERT(!"unreachable");
    return false;
}


// expressions
bool Semantic::visit(IdentifierExpressionAST *ast)
{
    if (ast->name) {
        if (Symbol *s = _scope->lookup(*ast->name))
            _expr.type = s->type();
        else
            _engine->error(ast->lineno, QString::fromLatin1("`%1' was not declared in this scope").arg(*ast->name));
    }
    return false;
}

bool Semantic::visit(LiteralExpressionAST *ast)
{
    if (ast->value) {
        _expr.isConstant = true;

        if (ast->value->at(0) == QLatin1Char('t') && *ast->value == QLatin1String("true"))
            _expr.type = _engine->boolType();
        else if (ast->value->at(0) == QLatin1Char('f') && *ast->value == QLatin1String("false"))
            _expr.type = _engine->boolType();
        else if (ast->value->endsWith(QLatin1Char('u')) || ast->value->endsWith(QLatin1Char('U')))
            _expr.type = _engine->uintType();
        else if (ast->value->endsWith(QLatin1String("lf")) || ast->value->endsWith(QLatin1String("LF")))
            _expr.type = _engine->doubleType();
        else if (ast->value->endsWith(QLatin1Char('f')) || ast->value->endsWith(QLatin1Char('f')) || ast->value->contains(QLatin1Char('.')))
            _expr.type = _engine->floatType();
        else
            _expr.type = _engine->intType();
    }
    return false;
}

bool Semantic::visit(BinaryExpressionAST *ast)
{
    ExprResult left = expression(ast->left);
    ExprResult right = expression(ast->right);
    _expr.isConstant = left.isConstant && right.isConstant;
    switch (ast->kind) {
    case AST::Kind_ArrayAccess:
        if (left.type) {
            if (const IndexType *idxType = left.type->asIndexType())
                _expr = idxType->indexElementType();
            else
                _engine->error(ast->lineno, QString::fromLatin1("Invalid type `%1' for array subscript").arg(left.type->toString()));
        }
        break;

    case AST::Kind_Modulus:
    case AST::Kind_Multiply:
    case AST::Kind_Divide:
    case AST::Kind_Plus:
    case AST::Kind_Minus:
    case AST::Kind_ShiftLeft:
    case AST::Kind_ShiftRight:
        _expr.type = left.type; // ### not exactly
        break;

    case AST::Kind_LessThan:
    case AST::Kind_GreaterThan:
    case AST::Kind_LessEqual:
    case AST::Kind_GreaterEqual:
    case AST::Kind_Equal:
    case AST::Kind_NotEqual:
    case AST::Kind_BitwiseAnd:
    case AST::Kind_BitwiseXor:
    case AST::Kind_BitwiseOr:
    case AST::Kind_LogicalAnd:
    case AST::Kind_LogicalXor:
    case AST::Kind_LogicalOr:
        _expr.type = _engine->boolType();
        break;

    case AST::Kind_Comma:
        _expr = right;
        break;
    }

    return false;
}

bool Semantic::visit(UnaryExpressionAST *ast)
{
    ExprResult expr = expression(ast->expr);
    _expr = expr;
    return false;
}

bool Semantic::visit(TernaryExpressionAST *ast)
{
    ExprResult first = expression(ast->first);
    ExprResult second = expression(ast->second);
    ExprResult third = expression(ast->third);
    _expr.isConstant = first.isConstant && second.isConstant && third.isConstant;
    _expr.type = second.type;
    return false;
}

bool Semantic::visit(AssignmentExpressionAST *ast)
{
    ExprResult variable = expression(ast->variable);
    ExprResult value = expression(ast->value);
    return false;
}

bool Semantic::visit(MemberAccessExpressionAST *ast)
{
    ExprResult expr = expression(ast->expr);
    if (expr.type && ast->field) {
        if (const VectorType *vecTy = expr.type->asVectorType()) {
            if (Symbol *s = vecTy->find(*ast->field))
                _expr.type = s->type();
            else
                _engine->error(ast->lineno, QString::fromLatin1("`%1' has no member named `%2'").arg(vecTy->name()).arg(*ast->field));
        } else if (const Struct *structTy = expr.type->asStructType()) {
            if (Symbol *s = structTy->find(*ast->field))
                _expr.type = s->type();
            else
                _engine->error(ast->lineno, QString::fromLatin1("`%1' has no member named `%2'").arg(structTy->name()).arg(*ast->field));
        } else {
            _engine->error(ast->lineno, QString::fromLatin1("Requested for member `%1', in a non class or vec instance").arg(*ast->field));
        }
    }
    return false;
}

bool Semantic::implicitCast(const Type *type, const Type *target) const
{
    if (! (type && target)) {
        return false;
    } else if (type->isEqualTo(target)) {
        return true;
    } else if (target->asUIntType() != 0) {
        return type->asIntType() != 0;
    } else if (target->asFloatType() != 0) {
        return type->asIntType() != 0 ||
                type->asUIntType() != 0;
    } else if (target->asDoubleType() != 0) {
        return type->asIntType() != 0 ||
                type->asUIntType() != 0 ||
                type->asFloatType() != 0;
    } else if (const VectorType *targetVecTy = target->asVectorType()) {
        if (const VectorType *vecTy = type->asVectorType()) {
            if (targetVecTy->dimension() == vecTy->dimension()) {
                const Type *targetElementType = targetVecTy->elementType();
                const Type *elementType = vecTy->elementType();

                if (targetElementType->asUIntType() != 0) {
                    // uvec* -> ivec*
                    return elementType->asIntType() != 0;
                } else if (targetElementType->asFloatType() != 0) {
                    // vec* -> ivec* | uvec*
                    return elementType->asIntType() != 0 ||
                            elementType->asUIntType() != 0;
                } else if (targetElementType->asDoubleType() != 0) {
                    // dvec* -> ivec* | uvec* | fvec*
                    return elementType->asIntType() != 0 ||
                            elementType->asUIntType() != 0 ||
                            elementType->asFloatType() != 0;
                }
            }
        }
    } else if (const MatrixType *targetMatTy = target->asMatrixType()) {
        if (const MatrixType *matTy = type->asMatrixType()) {
            if (targetMatTy->columns() == matTy->columns() &&
                    targetMatTy->rows() == matTy->rows()) {
                const Type *targetElementType = targetMatTy->elementType();
                const Type *elementType = matTy->elementType();

                if (targetElementType->asDoubleType() != 0) {
                    // dmat* -> mat*
                    return elementType->asFloatType() != 0;
                }
            }
        }
    }

    return false;
}

bool Semantic::visit(FunctionCallExpressionAST *ast)
{
    ExprResult expr = expression(ast->expr);
    ExprResult id = functionIdentifier(ast->id);
    QVector<ExprResult> actuals;
    for (List<ExpressionAST *> *it = ast->arguments; it; it = it->next) {
        ExprResult arg = expression(it->value);
        actuals.append(arg);
    }
    if (id.isValid()) {
        if (const Function *funTy = id.type->asFunctionType()) {
            if (actuals.size() < funTy->argumentCount())
                _engine->error(ast->lineno, QString::fromLatin1("not enough arguments"));
            else if (actuals.size() > funTy->argumentCount())
                _engine->error(ast->lineno, QString::fromLatin1("too many arguments"));
            _expr.type = funTy->returnType();
        } else if (const OverloadSet *overloads = id.type->asOverloadSetType()) {
            QVector<GLSL::Function *> candidates;
            foreach (GLSL::Function *f, overloads->functions()) {
                if (f->argumentCount() == actuals.size()) {
                    int argc = 0;
                    for (; argc < actuals.size(); ++argc) {
                        const Type *actualTy = actuals.at(argc).type;
                        const Type *argumentTy = f->argumentAt(argc)->type();
                        if (! implicitCast(actualTy, argumentTy))
                            break;
                    }

                    if (argc == actuals.size())
                        candidates.append(f);
                }
            }

            if (candidates.isEmpty()) {
                // ### error, unresolved call.
                Q_ASSERT(! overloads->functions().isEmpty());

                _expr.type = overloads->functions().first()->returnType();
            } else {
                _expr.type = candidates.first()->returnType();

                if (candidates.size() != 1) {
                    // ### error, ambiguous call
                }
            }
        } else {
            // called as constructor, e.g. vec2(a, b)
            _expr.type = id.type;
        }
    }

    return false;
}

bool Semantic::visit(DeclarationExpressionAST *ast)
{
    const Type *ty = type(ast->type);
    Q_UNUSED(ty);
    // ast->name
    ExprResult initializer = expression(ast->initializer);
    return false;
}


// statements
bool Semantic::visit(ExpressionStatementAST *ast)
{
    ExprResult expr = expression(ast->expr);
    return false;
}

bool Semantic::visit(CompoundStatementAST *ast)
{
    Block *block = _engine->newBlock(_scope);
    Scope *previousScope = switchScope(block);
    ast->symbol = block;
    for (List<StatementAST *> *it = ast->statements; it; it = it->next) {
        StatementAST *stmt = it->value;
        statement(stmt);
    }
    (void) switchScope(previousScope);
    return false;
}

bool Semantic::visit(IfStatementAST *ast)
{
    ExprResult condition = expression(ast->condition);
    statement(ast->thenClause);
    statement(ast->elseClause);
    return false;
}

bool Semantic::visit(WhileStatementAST *ast)
{
    ExprResult condition = expression(ast->condition);
    statement(ast->body);
    return false;
}

bool Semantic::visit(DoStatementAST *ast)
{
    statement(ast->body);
    ExprResult condition = expression(ast->condition);
    return false;
}

bool Semantic::visit(ForStatementAST *ast)
{
    statement(ast->init);
    ExprResult condition = expression(ast->condition);
    ExprResult increment = expression(ast->increment);
    statement(ast->body);
    return false;
}

bool Semantic::visit(JumpStatementAST *ast)
{
    Q_UNUSED(ast);
    return false;
}

bool Semantic::visit(ReturnStatementAST *ast)
{
    ExprResult expr = expression(ast->expr);
    return false;
}

bool Semantic::visit(SwitchStatementAST *ast)
{
    ExprResult expr = expression(ast->expr);
    statement(ast->body);
    return false;
}

bool Semantic::visit(CaseLabelStatementAST *ast)
{
    ExprResult expr = expression(ast->expr);
    return false;
}

bool Semantic::visit(DeclarationStatementAST *ast)
{
    declaration(ast->decl);
    return false;
}


// types
bool Semantic::visit(BasicTypeAST *ast)
{
    switch (ast->token) {
    case Parser::T_VOID:
        _type = _engine->voidType();
        break;

    case Parser::T_BOOL:
        _type = _engine->boolType();
        break;

    case Parser::T_INT:
        _type = _engine->intType();
        break;

    case Parser::T_UINT:
        _type = _engine->uintType();
        break;

    case Parser::T_FLOAT:
        _type = _engine->floatType();
        break;

    case Parser::T_DOUBLE:
        _type = _engine->doubleType();
        break;

    // bvec
    case Parser::T_BVEC2:
        _type = _engine->vectorType(_engine->boolType(), 2);
        break;

    case Parser::T_BVEC3:
        _type = _engine->vectorType(_engine->boolType(), 3);
        break;

    case Parser::T_BVEC4:
        _type = _engine->vectorType(_engine->boolType(), 4);
        break;

    // ivec
    case Parser::T_IVEC2:
        _type = _engine->vectorType(_engine->intType(), 2);
        break;

    case Parser::T_IVEC3:
        _type = _engine->vectorType(_engine->intType(), 3);
        break;

    case Parser::T_IVEC4:
        _type = _engine->vectorType(_engine->intType(), 4);
        break;

    // uvec
    case Parser::T_UVEC2:
        _type = _engine->vectorType(_engine->uintType(), 2);
        break;

    case Parser::T_UVEC3:
        _type = _engine->vectorType(_engine->uintType(), 3);
        break;

    case Parser::T_UVEC4:
        _type = _engine->vectorType(_engine->uintType(), 4);
        break;

    // vec
    case Parser::T_VEC2:
        _type = _engine->vectorType(_engine->floatType(), 2);
        break;

    case Parser::T_VEC3:
        _type = _engine->vectorType(_engine->floatType(), 3);
        break;

    case Parser::T_VEC4:
        _type = _engine->vectorType(_engine->floatType(), 4);
        break;

    // dvec
    case Parser::T_DVEC2:
        _type = _engine->vectorType(_engine->doubleType(), 2);
        break;

    case Parser::T_DVEC3:
        _type = _engine->vectorType(_engine->doubleType(), 3);
        break;

    case Parser::T_DVEC4:
        _type = _engine->vectorType(_engine->doubleType(), 4);
        break;

    // mat2
    case Parser::T_MAT2:
    case Parser::T_MAT2X2:
        _type = _engine->matrixType(_engine->floatType(), 2, 2);
        break;

    case Parser::T_MAT2X3:
        _type = _engine->matrixType(_engine->floatType(), 2, 3);
        break;

    case Parser::T_MAT2X4:
        _type = _engine->matrixType(_engine->floatType(), 2, 4);
        break;

    // mat3
    case Parser::T_MAT3X2:
        _type = _engine->matrixType(_engine->floatType(), 3, 2);
        break;

    case Parser::T_MAT3:
    case Parser::T_MAT3X3:
        _type = _engine->matrixType(_engine->floatType(), 3, 3);
        break;

    case Parser::T_MAT3X4:
        _type = _engine->matrixType(_engine->floatType(), 3, 4);
        break;

    // mat4
    case Parser::T_MAT4X2:
        _type = _engine->matrixType(_engine->floatType(), 4, 2);
        break;

    case Parser::T_MAT4X3:
        _type = _engine->matrixType(_engine->floatType(), 4, 3);
        break;

    case Parser::T_MAT4:
    case Parser::T_MAT4X4:
        _type = _engine->matrixType(_engine->floatType(), 4, 4);
        break;


    // dmat2
    case Parser::T_DMAT2:
    case Parser::T_DMAT2X2:
        _type = _engine->matrixType(_engine->doubleType(), 2, 2);
        break;

    case Parser::T_DMAT2X3:
        _type = _engine->matrixType(_engine->doubleType(), 2, 3);
        break;

    case Parser::T_DMAT2X4:
        _type = _engine->matrixType(_engine->doubleType(), 2, 4);
        break;

    // dmat3
    case Parser::T_DMAT3X2:
        _type = _engine->matrixType(_engine->doubleType(), 3, 2);
        break;

    case Parser::T_DMAT3:
    case Parser::T_DMAT3X3:
        _type = _engine->matrixType(_engine->doubleType(), 3, 3);
        break;

    case Parser::T_DMAT3X4:
        _type = _engine->matrixType(_engine->doubleType(), 3, 4);
        break;

    // dmat4
    case Parser::T_DMAT4X2:
        _type = _engine->matrixType(_engine->doubleType(), 4, 2);
        break;

    case Parser::T_DMAT4X3:
        _type = _engine->matrixType(_engine->doubleType(), 4, 3);
        break;

    case Parser::T_DMAT4:
    case Parser::T_DMAT4X4:
        _type = _engine->matrixType(_engine->doubleType(), 4, 4);
        break;

    // samplers
    case Parser::T_SAMPLER1D:
    case Parser::T_SAMPLER2D:
    case Parser::T_SAMPLER3D:
    case Parser::T_SAMPLERCUBE:
    case Parser::T_SAMPLER1DSHADOW:
    case Parser::T_SAMPLER2DSHADOW:
    case Parser::T_SAMPLERCUBESHADOW:
    case Parser::T_SAMPLER1DARRAY:
    case Parser::T_SAMPLER2DARRAY:
    case Parser::T_SAMPLER1DARRAYSHADOW:
    case Parser::T_SAMPLER2DARRAYSHADOW:
    case Parser::T_SAMPLERCUBEARRAY:
    case Parser::T_SAMPLERCUBEARRAYSHADOW:
    case Parser::T_SAMPLER2DRECT:
    case Parser::T_SAMPLER2DRECTSHADOW:
    case Parser::T_SAMPLERBUFFER:
    case Parser::T_SAMPLER2DMS:
    case Parser::T_SAMPLER2DMSARRAY:
    case Parser::T_ISAMPLER1D:
    case Parser::T_ISAMPLER2D:
    case Parser::T_ISAMPLER3D:
    case Parser::T_ISAMPLERCUBE:
    case Parser::T_ISAMPLER1DARRAY:
    case Parser::T_ISAMPLER2DARRAY:
    case Parser::T_ISAMPLERCUBEARRAY:
    case Parser::T_ISAMPLER2DRECT:
    case Parser::T_ISAMPLERBUFFER:
    case Parser::T_ISAMPLER2DMS:
    case Parser::T_ISAMPLER2DMSARRAY:
    case Parser::T_USAMPLER1D:
    case Parser::T_USAMPLER2D:
    case Parser::T_USAMPLER3D:
    case Parser::T_USAMPLERCUBE:
    case Parser::T_USAMPLER1DARRAY:
    case Parser::T_USAMPLER2DARRAY:
    case Parser::T_USAMPLERCUBEARRAY:
    case Parser::T_USAMPLER2DRECT:
    case Parser::T_USAMPLERBUFFER:
    case Parser::T_USAMPLER2DMS:
    case Parser::T_USAMPLER2DMSARRAY:
        _type = _engine->samplerType(ast->token);
        break;

    default:
        _engine->error(ast->lineno, QString::fromLatin1("Unknown type `%1'").arg(QLatin1String(GLSLParserTable::spell[ast->token])));
    }

    return false;
}

bool Semantic::visit(NamedTypeAST *ast)
{
    if (ast->name) {
        if (Symbol *s = _scope->lookup(*ast->name)) {
            if (Struct *ty = s->asStruct()) {
                _type = ty;
                return false;
            }
        }
        _engine->error(ast->lineno, QString::fromLatin1("Undefined type `%1'").arg(*ast->name));
    }

    return false;
}

bool Semantic::visit(ArrayTypeAST *ast)
{
    const Type *elementType = type(ast->elementType);
    Q_UNUSED(elementType);
    ExprResult size = expression(ast->size);
    _type = _engine->arrayType(elementType); // ### ignore the size for now
    return false;
}

bool Semantic::visit(StructTypeAST *ast)
{
    Struct *s = _engine->newStruct(_scope);
    if (ast->name)
        s->setName(*ast->name);
    if (Scope *e = s->scope())
        e->add(s);
    Scope *previousScope = switchScope(s);
    for (List<StructTypeAST::Field *> *it = ast->fields; it; it = it->next) {
        StructTypeAST::Field *f = it->value;
        if (Symbol *member = field(f))
            s->add(member);
    }
    (void) switchScope(previousScope);
    return false;
}

bool Semantic::visit(QualifiedTypeAST *ast)
{
    _type = type(ast->type);
    for (List<LayoutQualifierAST *> *it = ast->layout_list; it; it = it->next) {
        LayoutQualifierAST *q = it->value;
        // q->name;
        // q->number;
        Q_UNUSED(q);
    }
    return false;
}


// declarations
bool Semantic::visit(PrecisionDeclarationAST *ast)
{
    const Type *ty = type(ast->type);
    Q_UNUSED(ty);
    return false;
}

bool Semantic::visit(ParameterDeclarationAST *ast)
{
    Q_UNUSED(ast);
    Q_ASSERT(!"unreachable");
    return false;
}

bool Semantic::visit(VariableDeclarationAST *ast)
{
    if (!ast->type)
        return false;

    const Type *ty = type(ast->type);
    ExprResult initializer = expression(ast->initializer);
    if (ast->name) {
        QualifiedTypeAST *qtype = ast->type->asQualifiedType();
        int qualifiers = 0;
        if (qtype)
            qualifiers = qtype->qualifiers;
        Variable *var = _engine->newVariable(_scope, *ast->name, ty, qualifiers);
        _scope->add(var);
    }
    return false;
}

bool Semantic::visit(TypeDeclarationAST *ast)
{
    const Type *ty = type(ast->type);
    Q_UNUSED(ty);
    return false;
}

bool Semantic::visit(TypeAndVariableDeclarationAST *ast)
{
    declaration(ast->typeDecl);
    declaration(ast->varDecl);
    return false;
}

bool Semantic::visit(InvariantDeclarationAST *ast)
{
    Q_UNUSED(ast);
    return false;
}

bool Semantic::visit(InitDeclarationAST *ast)
{
    for (List<DeclarationAST *> *it = ast->decls; it; it = it->next) {
        DeclarationAST *decl = it->value;
        declaration(decl);
    }
    return false;
}

bool Semantic::visit(FunctionDeclarationAST *ast)
{
    Function *fun = _engine->newFunction(_scope);
    if (ast->name)
        fun->setName(*ast->name);

    fun->setReturnType(type(ast->returnType));

    for (List<ParameterDeclarationAST *> *it = ast->params; it; it = it->next) {
        ParameterDeclarationAST *decl = it->value;
        parameterDeclaration(decl, fun);
    }

    if (Scope *enclosingScope = fun->scope())
        enclosingScope->add(fun);

    Scope *previousScope = switchScope(fun);
    statement(ast->body);
    (void) switchScope(previousScope);
    return false;
}

