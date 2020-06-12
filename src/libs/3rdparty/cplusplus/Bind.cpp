// Copyright (c) 2008 Roberto Raggi <roberto.raggi@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "Bind.h"
#include "AST.h"
#include "TranslationUnit.h"
#include "Control.h"
#include "Names.h"
#include "Symbols.h"
#include "CoreTypes.h"
#include "Literals.h"
#include "Scope.h"

#include "cppassert.h"

#include <algorithm>
#include <vector>
#include <string>
#include <memory>
#include <sstream>

using namespace CPlusPlus;

const int Bind::kMaxDepth(100);

Bind::Bind(TranslationUnit *unit)
    : ASTVisitor(unit),
      _scope(nullptr),
      _expression(nullptr),
      _name(nullptr),
      _declaratorId(nullptr),
      _visibility(Symbol::Public),
      _objcVisibility(Symbol::Public),
      _methodKey(Function::NormalMethod),
      _skipFunctionBodies(false),
      _depth(0)
{
}

bool Bind::skipFunctionBodies() const
{
    return _skipFunctionBodies;
}

void Bind::setSkipFunctionBodies(bool skipFunctionBodies)
{
    _skipFunctionBodies = skipFunctionBodies;
}

int Bind::location(DeclaratorAST *ast, int defaultLocation) const
{
    if (! ast)
        return defaultLocation;

    else if (ast->core_declarator)
        return location(ast->core_declarator, defaultLocation);

    return ast->firstToken();
}

int Bind::location(CoreDeclaratorAST *ast, int defaultLocation) const
{
    if (! ast)
        return defaultLocation;

    else if (NestedDeclaratorAST *nested = ast->asNestedDeclarator())
        return location(nested->declarator, defaultLocation);

    else if (DeclaratorIdAST *id = ast->asDeclaratorId())
        return location(id->name, defaultLocation);

    return ast->firstToken();
}

int Bind::location(NameAST *name, int defaultLocation) const
{
    if (! name)
        return defaultLocation;

    else if (DestructorNameAST *dtor = name->asDestructorName())
        return location(dtor->unqualified_name, defaultLocation);

    else if (TemplateIdAST *templId = name->asTemplateId())
        return templId->identifier_token;

    else if (QualifiedNameAST *q = name->asQualifiedName()) {
        if (q->unqualified_name)
            return location(q->unqualified_name, defaultLocation);
    }

    return name->firstToken();
}

void Bind::setDeclSpecifiers(Symbol *symbol, const FullySpecifiedType &declSpecifiers)
{
    if (! symbol)
        return;

    int storage = Symbol::NoStorage;

    if (declSpecifiers.isFriend())
        storage = Symbol::Friend;
    else if (declSpecifiers.isAuto())
        storage = Symbol::Auto;
    else if (declSpecifiers.isRegister())
        storage = Symbol::Register;
    else if (declSpecifiers.isStatic())
        storage = Symbol::Static;
    else if (declSpecifiers.isExtern())
        storage = Symbol::Extern;
    else if (declSpecifiers.isMutable())
        storage = Symbol::Mutable;
    else if (declSpecifiers.isTypedef())
        storage = Symbol::Typedef;

    symbol->setStorage(storage);

    if (Function *funTy = symbol->asFunction()) {
        if (declSpecifiers.isVirtual())
            funTy->setVirtual(true);
    }

    if (declSpecifiers.isDeprecated())
        symbol->setDeprecated(true);

    if (declSpecifiers.isUnavailable())
        symbol->setUnavailable(true);
}

Scope *Bind::switchScope(Scope *scope)
{
    if (! scope)
        return _scope;

    std::swap(_scope, scope);
    return scope;
}

int Bind::switchVisibility(int visibility)
{
    std::swap(_visibility, visibility);
    return visibility;
}

int Bind::switchObjCVisibility(int visibility)
{
    std::swap(_objcVisibility, visibility);
    return visibility;
}

int Bind::switchMethodKey(int methodKey)
{
    std::swap(_methodKey, methodKey);
    return methodKey;
}

void Bind::operator()(TranslationUnitAST *ast, Namespace *globalNamespace)
{
    Scope *previousScope = switchScope(globalNamespace);
    translationUnit(ast);
    (void) switchScope(previousScope);
}

void Bind::operator()(DeclarationAST *ast, Scope *scope)
{
    Scope *previousScope = switchScope(scope);
    declaration(ast);
    (void) switchScope(previousScope);
}

void Bind::operator()(StatementAST *ast, Scope *scope)
{
    Scope *previousScope = switchScope(scope);
    statement(ast);
    (void) switchScope(previousScope);
}

FullySpecifiedType Bind::operator()(ExpressionAST *ast, Scope *scope)
{
    Scope *previousScope = switchScope(scope);
    FullySpecifiedType ty = expression(ast);
    (void) switchScope(previousScope);
    return ty;
}

FullySpecifiedType Bind::operator()(NewTypeIdAST *ast, Scope *scope)
{
    Scope *previousScope = switchScope(scope);
    FullySpecifiedType ty = newTypeId(ast);
    (void) switchScope(previousScope);
    return ty;
}

void Bind::statement(StatementAST *ast)
{
    accept(ast);
}

Bind::ExpressionTy Bind::expression(ExpressionAST *ast)
{
    ExpressionTy value = ExpressionTy();
    std::swap(_expression, value);
    accept(ast);
    std::swap(_expression, value);
    return value;
}

void Bind::declaration(DeclarationAST *ast)
{
    accept(ast);
}

const Name *Bind::name(NameAST *ast)
{
    const Name *value = nullptr;
    std::swap(_name, value);
    accept(ast);
    std::swap(_name, value);
    return value;
}

FullySpecifiedType Bind::specifier(SpecifierAST *ast, const FullySpecifiedType &init)
{
    FullySpecifiedType value = init;
    std::swap(_type, value);
    accept(ast);
    std::swap(_type, value);
    return value;
}

FullySpecifiedType Bind::ptrOperator(PtrOperatorAST *ast, const FullySpecifiedType &init)
{
    FullySpecifiedType value = init;
    std::swap(_type, value);
    accept(ast);
    std::swap(_type, value);
    return value;
}

FullySpecifiedType Bind::coreDeclarator(CoreDeclaratorAST *ast, const FullySpecifiedType &init)
{
    FullySpecifiedType value = init;
    std::swap(_type, value);
    accept(ast);
    std::swap(_type, value);
    return value;
}

FullySpecifiedType Bind::postfixDeclarator(PostfixDeclaratorAST *ast, const FullySpecifiedType &init)
{
    FullySpecifiedType value = init;
    std::swap(_type, value);
    accept(ast);
    std::swap(_type, value);
    return value;
}

bool Bind::preVisit(AST *)
{
    ++_depth;
    if (_depth > kMaxDepth)
        return false;
    return true;
}

void Bind::postVisit(AST *)
{
    --_depth;
}

// AST
bool Bind::visit(ObjCSelectorArgumentAST *ast)
{
    (void) ast;
    CPP_CHECK(!"unreachable");
    return false;
}

const Name *Bind::objCSelectorArgument(ObjCSelectorArgumentAST *ast, bool *hasArg)
{
    if (! (ast && ast->name_token))
        return nullptr;

    if (ast->colon_token)
        *hasArg = true;

    return identifier(ast->name_token);
}

bool Bind::visit(GnuAttributeAST *ast)
{
    (void) ast;
    CPP_CHECK(!"unreachable");
    return false;
}

void Bind::attribute(GnuAttributeAST *ast)
{
    if (! ast)
        return;

    // int identifier_token = ast->identifier_token;
    if (const Identifier *id = identifier(ast->identifier_token)) {
        if (id == control()->deprecatedId())
            _type.setDeprecated(true);
        else if (id == control()->unavailableId())
            _type.setUnavailable(true);
    }

    // int lparen_token = ast->lparen_token;
    // int tag_token = ast->tag_token;
    for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
        ExpressionTy value = this->expression(it->value);
    }
    // int rparen_token = ast->rparen_token;
}

bool Bind::visit(DeclaratorAST *ast)
{
    (void) ast;
    CPP_CHECK(!"unreachable");
    return false;
}

FullySpecifiedType Bind::declarator(DeclaratorAST *ast, const FullySpecifiedType &init, DeclaratorIdAST **declaratorId)
{
    FullySpecifiedType type = init;

    if (! ast)
        return type;

    std::swap(_declaratorId, declaratorId);
    bool isAuto = false;
    const bool cxx11Enabled = translationUnit()->languageFeatures().cxx11Enabled;
    if (cxx11Enabled)
        isAuto = type.isAuto();

    for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
        type = this->specifier(it->value, type);
        if (type.isAuto())
            isAuto = true;
    }
    for (PtrOperatorListAST *it = ast->ptr_operator_list; it; it = it->next) {
        type = this->ptrOperator(it->value, type);
    }
    for (PostfixDeclaratorListAST *it = ast->postfix_declarator_list; it; it = it->next) {
        type = this->postfixDeclarator(it->value, type);
    }
    type = this->coreDeclarator(ast->core_declarator, type);
    for (SpecifierListAST *it = ast->post_attribute_list; it; it = it->next) {
        type = this->specifier(it->value, type);
        if (type.isAuto())
            isAuto = true;
    }
    if (!type->isFunctionType()) {
        ExpressionTy initializer = this->expression(ast->initializer);
        if (cxx11Enabled && isAuto) {
            type = initializer;
            type.setAuto(true);
        }
    }

    std::swap(_declaratorId, declaratorId);
    return type;
}

bool Bind::visit(QtPropertyDeclarationItemAST *ast)
{
    (void) ast;
    CPP_CHECK(!"unreachable");
    return false;
}

bool Bind::visit(QtInterfaceNameAST *ast)
{
    (void) ast;
    CPP_CHECK(!"unreachable");
    return false;
}

void Bind::qtInterfaceName(QtInterfaceNameAST *ast)
{
    if (! ast)
        return;

    /*const Name *interface_name =*/ this->name(ast->interface_name);
    for (NameListAST *it = ast->constraint_list; it; it = it->next) {
        /*const Name *value =*/ this->name(it->value);
    }
}

bool Bind::visit(BaseSpecifierAST *ast)
{
    (void) ast;
    CPP_CHECK(!"unreachable");
    return false;
}

void Bind::baseSpecifier(BaseSpecifierAST *ast, int colon_token, Class *klass)
{
    if (! ast)
        return;

    int sourceLocation = location(ast->name, ast->firstToken());
    if (! sourceLocation)
        sourceLocation = std::max(colon_token, klass->sourceLocation());

    const Name *baseClassName = this->name(ast->name);
    BaseClass *baseClass = control()->newBaseClass(sourceLocation, baseClassName);
    if (ast->virtual_token)
        baseClass->setVirtual(true);
    if (ast->access_specifier_token) {
        const int visibility = visibilityForAccessSpecifier(tokenKind(ast->access_specifier_token));
        baseClass->setVisibility(visibility); // ### well, not exactly.
    }
    if (ast->ellipsis_token)
        baseClass->setVariadic(true);
    klass->addBaseClass(baseClass);
    ast->symbol = baseClass;
}

bool Bind::visit(CtorInitializerAST *ast)
{
    (void) ast;
    CPP_CHECK(!"unreachable");
    return false;
}

void Bind::ctorInitializer(CtorInitializerAST *ast, Function *fun)
{
    if (! ast)
        return;

    // int colon_token = ast->colon_token;
    for (MemInitializerListAST *it = ast->member_initializer_list; it; it = it->next) {
        this->memInitializer(it->value, fun);
    }
    // int dot_dot_dot_token = ast->dot_dot_dot_token;
}

bool Bind::visit(EnumeratorAST *ast)
{
    (void) ast;
    CPP_CHECK(!"unreachable");
    return false;
}

namespace {

bool isInteger(const StringLiteral *stringLiteral)
{
    const int size = stringLiteral->size();
    const char *chars = stringLiteral->chars();
    int i = 0;
    if (chars[i] == '-')
        ++i;
    for (; i < size; ++i) {
        if (!isdigit(chars[i]))
            return false;
    }
    return true;
}

bool stringLiteralToInt(const StringLiteral *stringLiteral, int *output)
{
    if (!output)
        return false;

    if (!isInteger(stringLiteral)) {
        *output = 0;
        return false;
    }

    std::stringstream ss(std::string(stringLiteral->chars(), stringLiteral->size()));
    const bool ok = !(ss >> *output).fail();
    if (!ok)
        *output = 0;

    return ok;
}

void calculateConstantValue(const Symbol *symbol, EnumeratorDeclaration *e, Control *control)
{
    if (symbol) {
        if (const Declaration *decl = symbol->asDeclaration()) {
            if (const EnumeratorDeclaration *previousEnumDecl = decl->asEnumeratorDeclarator()) {
                if (const StringLiteral *constantValue = previousEnumDecl->constantValue()) {
                    int constantValueAsInt = 0;
                    if (stringLiteralToInt(constantValue, &constantValueAsInt)) {
                        ++constantValueAsInt;
                        const std::string buffer
                                = std::to_string(static_cast<long long>(constantValueAsInt));
                        e->setConstantValue(control->stringLiteral(buffer.c_str(),
                                                                   int(buffer.size())));
                    }
                }
            }
        }
    }
}

const StringLiteral *valueOfEnumerator(const Enum *e, const Identifier *value) {
    const int enumMemberCount = e->memberCount();
    for (int i = 0; i < enumMemberCount; ++i) {
        const Symbol *member = e->memberAt(i);
        if (const Declaration *decl = member->asDeclaration()) {
            if (const EnumeratorDeclaration *enumDecl = decl->asEnumeratorDeclarator()) {
                if (const Name *enumDeclName = enumDecl->name()) {
                    if (const Identifier *enumDeclIdentifier = enumDeclName->identifier()) {
                        if (enumDeclIdentifier->equalTo(value))
                            return enumDecl->constantValue();
                    }
                }
            }
        }
    }
    return nullptr;
}

} // anonymous namespace

void Bind::enumerator(EnumeratorAST *ast, Enum *symbol)
{
    (void) symbol;

    if (! ast)
        return;

    // int identifier_token = ast->identifier_token;
    // int equal_token = ast->equal_token;
    /*ExpressionTy expression =*/ this->expression(ast->expression);

    if (ast->identifier_token) {
        const Name *name = identifier(ast->identifier_token);
        EnumeratorDeclaration *e = control()->newEnumeratorDeclaration(ast->identifier_token, name);
        e->setType(control()->integerType(IntegerType::Int)); // ### introduce IntegerType::Enumerator

        if (ExpressionAST *expr = ast->expression) {
            const int firstToken = expr->firstToken();
            const int lastToken = expr->lastToken();
            const StringLiteral *constantValue = asStringLiteral(expr);
            const StringLiteral *resolvedValue = nullptr;
            if (lastToken - firstToken == 1) {
                if (const Identifier *constantId = identifier(firstToken))
                    resolvedValue = valueOfEnumerator(symbol, constantId);
            }
            e->setConstantValue(resolvedValue ? resolvedValue : constantValue);
        } else if (!symbol->isEmpty()) {
            calculateConstantValue(*(symbol->memberEnd()-1), e, control());
        } else {
            e->setConstantValue(control()->stringLiteral("0", 1));
        }

        symbol->addMember(e);
    }
}

bool Bind::visit(DynamicExceptionSpecificationAST *ast)
{
    (void) ast;
    CPP_CHECK(!"unreachable");
    return false;
}

FullySpecifiedType Bind::exceptionSpecification(ExceptionSpecificationAST *ast, const FullySpecifiedType &init)
{
    FullySpecifiedType type = init;

    if (! ast)
        return type;

    if (DynamicExceptionSpecificationAST *dyn = ast->asDynamicExceptionSpecification()) {
        // int throw_token = ast->throw_token;
        // int lparen_token = ast->lparen_token;
        // int dot_dot_dot_token = ast->dot_dot_dot_token;
        for (ExpressionListAST *it = dyn->type_id_list; it; it = it->next) {
            /*ExpressionTy value =*/ this->expression(it->value);
        }
    } else if (NoExceptSpecificationAST *no = ast->asNoExceptSpecification()) {
        /*ExpressionTy value =*/ this->expression(no->expression);
    }
    // int rparen_token = ast->rparen_token;
    return type;
}

bool Bind::visit(MemInitializerAST *ast)
{
    (void) ast;
    CPP_CHECK(!"unreachable");
    return false;
}

void Bind::memInitializer(MemInitializerAST *ast, Function *fun)
{
    if (! ast)
        return;

    /*const Name *name =*/ this->name(ast->name);

    Scope *previousScope = switchScope(fun);
    this->expression(ast->expression);
    (void) switchScope(previousScope);
}

bool Bind::visit(NestedNameSpecifierAST *ast)
{
    (void) ast;
    CPP_CHECK(!"unreachable");
    return false;
}

const Name *Bind::nestedNameSpecifier(NestedNameSpecifierAST *ast)
{
    if (! ast)
        return nullptr;

    const Name *class_or_namespace_name = this->name(ast->class_or_namespace_name);
    return class_or_namespace_name;
}

bool Bind::visit(ExpressionListParenAST *ast)
{
    // int lparen_token = ast->lparen_token;
    for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
        /*ExpressionTy value =*/ this->expression(it->value);
    }
    // int rparen_token = ast->rparen_token;

    return false;
}

void Bind::newPlacement(ExpressionListParenAST *ast)
{
    if (! ast)
        return;

    // int lparen_token = ast->lparen_token;
    for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
        ExpressionTy value = this->expression(it->value);
    }
    // int rparen_token = ast->rparen_token;
}

bool Bind::visit(NewArrayDeclaratorAST *ast)
{
    (void) ast;
    CPP_CHECK(!"unreachable");
    return false;
}

FullySpecifiedType Bind::newArrayDeclarator(NewArrayDeclaratorAST *ast, const FullySpecifiedType &init)
{
    FullySpecifiedType type = init;

    if (! ast)
        return type;

    // int lbracket_token = ast->lbracket_token;
    ExpressionTy expression = this->expression(ast->expression);
    // int rbracket_token = ast->rbracket_token;
    return type;
}

bool Bind::visit(NewTypeIdAST *ast)
{
    (void) ast;
    CPP_CHECK(!"unreachable");
    return false;
}

FullySpecifiedType Bind::newTypeId(NewTypeIdAST *ast)
{
    FullySpecifiedType type;

    if (! ast)
        return type;


    for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    for (PtrOperatorListAST *it = ast->ptr_operator_list; it; it = it->next) {
        type = this->ptrOperator(it->value, type);
    }
    for (NewArrayDeclaratorListAST *it = ast->new_array_declarator_list; it; it = it->next) {
        type = this->newArrayDeclarator(it->value, type);
    }
    return type;
}

bool Bind::visit(OperatorAST *ast)
{
    (void) ast;
    CPP_CHECK(!"unreachable");
    return false;
}

OperatorNameId::Kind Bind::cppOperator(OperatorAST *ast)
{
    OperatorNameId::Kind kind = OperatorNameId::InvalidOp;

    if (! ast)
        return kind;

    // int op_token = ast->op_token;
    // int open_token = ast->open_token;
    // int close_token = ast->close_token;

    switch (tokenKind(ast->op_token)) {
    case T_NEW:
        if (ast->open_token)
            kind = OperatorNameId::NewArrayOp;
        else
            kind = OperatorNameId::NewOp;
        break;

    case T_DELETE:
        if (ast->open_token)
            kind = OperatorNameId::DeleteArrayOp;
        else
            kind = OperatorNameId::DeleteOp;
        break;

    case T_PLUS:
        kind = OperatorNameId::PlusOp;
        break;

    case T_MINUS:
        kind = OperatorNameId::MinusOp;
        break;

    case T_STAR:
        kind = OperatorNameId::StarOp;
        break;

    case T_SLASH:
        kind = OperatorNameId::SlashOp;
        break;

    case T_PERCENT:
        kind = OperatorNameId::PercentOp;
        break;

    case T_CARET:
        kind = OperatorNameId::CaretOp;
        break;

    case T_AMPER:
        kind = OperatorNameId::AmpOp;
        break;

    case T_PIPE:
        kind = OperatorNameId::PipeOp;
        break;

    case T_TILDE:
        kind = OperatorNameId::TildeOp;
        break;

    case T_EXCLAIM:
        kind = OperatorNameId::ExclaimOp;
        break;

    case T_EQUAL:
        kind = OperatorNameId::EqualOp;
        break;

    case T_LESS:
        kind = OperatorNameId::LessOp;
        break;

    case T_GREATER:
        kind = OperatorNameId::GreaterOp;
        break;

    case T_PLUS_EQUAL:
        kind = OperatorNameId::PlusEqualOp;
        break;

    case T_MINUS_EQUAL:
        kind = OperatorNameId::MinusEqualOp;
        break;

    case T_STAR_EQUAL:
        kind = OperatorNameId::StarEqualOp;
        break;

    case T_SLASH_EQUAL:
        kind = OperatorNameId::SlashEqualOp;
        break;

    case T_PERCENT_EQUAL:
        kind = OperatorNameId::PercentEqualOp;
        break;

    case T_CARET_EQUAL:
        kind = OperatorNameId::CaretEqualOp;
        break;

    case T_AMPER_EQUAL:
        kind = OperatorNameId::AmpEqualOp;
        break;

    case T_PIPE_EQUAL:
        kind = OperatorNameId::PipeEqualOp;
        break;

    case T_LESS_LESS:
        kind = OperatorNameId::LessLessOp;
        break;

    case T_GREATER_GREATER:
        kind = OperatorNameId::GreaterGreaterOp;
        break;

    case T_LESS_LESS_EQUAL:
        kind = OperatorNameId::LessLessEqualOp;
        break;

    case T_GREATER_GREATER_EQUAL:
        kind = OperatorNameId::GreaterGreaterEqualOp;
        break;

    case T_EQUAL_EQUAL:
        kind = OperatorNameId::EqualEqualOp;
        break;

    case T_EXCLAIM_EQUAL:
        kind = OperatorNameId::ExclaimEqualOp;
        break;

    case T_LESS_EQUAL:
        kind = OperatorNameId::LessEqualOp;
        break;

    case T_GREATER_EQUAL:
        kind = OperatorNameId::GreaterEqualOp;
        break;

    case T_AMPER_AMPER:
        kind = OperatorNameId::AmpAmpOp;
        break;

    case T_PIPE_PIPE:
        kind = OperatorNameId::PipePipeOp;
        break;

    case T_PLUS_PLUS:
        kind = OperatorNameId::PlusPlusOp;
        break;

    case T_MINUS_MINUS:
        kind = OperatorNameId::MinusMinusOp;
        break;

    case T_COMMA:
        kind = OperatorNameId::CommaOp;
        break;

    case T_ARROW_STAR:
        kind = OperatorNameId::ArrowStarOp;
        break;

    case T_ARROW:
        kind = OperatorNameId::ArrowOp;
        break;

    case T_LPAREN:
        kind = OperatorNameId::FunctionCallOp;
        break;

    case T_LBRACKET:
        kind = OperatorNameId::ArrayAccessOp;
        break;

    default:
        kind = OperatorNameId::InvalidOp;
    } // switch

    return kind;
}

bool Bind::visit(ParameterDeclarationClauseAST *ast)
{
    (void) ast;
    CPP_CHECK(!"unreachable");
    return false;
}

void Bind::parameterDeclarationClause(ParameterDeclarationClauseAST *ast, int lparen_token, Function *fun)
{
    if (! ast)
        return;

    if (! fun) {
        translationUnit()->warning(lparen_token, "undefined function");
        return;
    }

    Scope *previousScope = switchScope(fun);

    for (ParameterDeclarationListAST *it = ast->parameter_declaration_list; it; it = it->next) {
        this->declaration(it->value);

        // Check for '...' in last parameter declarator for variadic template
        // (i.e. template<class ... T> void foo(T ... args);)
        // those last dots are part of parameter declarator, not the parameter declaration clause
        if (! it->next
            && it->value->declarator != nullptr
            && it->value->declarator->core_declarator != nullptr){
            DeclaratorIdAST* declId = it->value->declarator->core_declarator->asDeclaratorId();
            if (declId && declId->dot_dot_dot_token != 0){
                fun->setVariadic(true);
                fun->setVariadicTemplate(true);
            }
        }
    }

    if (ast->dot_dot_dot_token)
        fun->setVariadic(true);

    (void) switchScope(previousScope);
}

bool Bind::visit(TranslationUnitAST *ast)
{
    (void) ast;
    CPP_CHECK(!"unreachable");
    return false;
}

void Bind::translationUnit(TranslationUnitAST *ast)
{
    if (! ast)
        return;

    for (DeclarationListAST *it = ast->declaration_list; it; it = it->next) {
        this->declaration(it->value);
    }
}

bool Bind::visit(ObjCProtocolRefsAST *ast)
{
    (void) ast;
    CPP_CHECK(!"unreachable");
    return false;
}

void Bind::objCProtocolRefs(ObjCProtocolRefsAST *ast, Symbol *objcClassOrProtocol)
{
    if (! ast)
        return;

    for (NameListAST *it = ast->identifier_list; it; it = it->next) {
        const Name *protocolName = this->name(it->value);
        ObjCBaseProtocol *baseProtocol = control()->newObjCBaseProtocol(it->value->firstToken(), protocolName);
        if (ObjCClass *klass = objcClassOrProtocol->asObjCClass())
            klass->addProtocol(baseProtocol);
        else if (ObjCProtocol *proto = objcClassOrProtocol->asObjCProtocol())
            proto->addProtocol(baseProtocol);
    }
}

bool Bind::visit(ObjCMessageArgumentAST *ast)
{
    (void) ast;
    CPP_CHECK(!"unreachable");
    return false;
}

void Bind::objCMessageArgument(ObjCMessageArgumentAST *ast)
{
    if (! ast)
        return;

    ExpressionTy parameter_value_expression = this->expression(ast->parameter_value_expression);
}

bool Bind::visit(ObjCTypeNameAST *ast)
{
    (void) ast;
    CPP_CHECK(!"unreachable");
    return false;
}

FullySpecifiedType Bind::objCTypeName(ObjCTypeNameAST *ast)
{
    if (! ast)
        return FullySpecifiedType();

    // int type_qualifier_token = ast->type_qualifier_token;
    ExpressionTy type_id = this->expression(ast->type_id);
    return type_id;
}

bool Bind::visit(ObjCInstanceVariablesDeclarationAST *ast)
{
    (void) ast;
    CPP_CHECK(!"unreachable");
    return false;
}

void Bind::objCInstanceVariablesDeclaration(ObjCInstanceVariablesDeclarationAST *ast, ObjCClass *klass)
{
    (void) klass;

    if (! ast)
        return;

    // int lbrace_token = ast->lbrace_token;
    for (DeclarationListAST *it = ast->instance_variable_list; it; it = it->next) {
        this->declaration(it->value);
    }
    // int rbrace_token = ast->rbrace_token;
}

bool Bind::visit(ObjCPropertyAttributeAST *ast)
{
    (void) ast;
    CPP_CHECK(!"unreachable");
    return false;
}

void Bind::objCPropertyAttribute(ObjCPropertyAttributeAST *ast)
{
    if (! ast)
        return;

    // int attribute_identifier_token = ast->attribute_identifier_token;
    // int equals_token = ast->equals_token;
    /*const Name *method_selector =*/ this->name(ast->method_selector);
}

bool Bind::visit(ObjCMessageArgumentDeclarationAST *ast)
{
    (void) ast;
    CPP_CHECK(!"unreachable");
    return false;
}

void Bind::objCMessageArgumentDeclaration(ObjCMessageArgumentDeclarationAST *ast, ObjCMethod *method)
{
    if (! ast)
        return;

    FullySpecifiedType type = this->objCTypeName(ast->type_name);

    for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }

    const Name *param_name = this->name(ast->param_name);
    Argument *arg = control()->newArgument(location(ast->param_name, ast->firstToken()), param_name);
    arg->setType(type);
    ast->argument = arg;
    method->addMember(arg);
}

bool Bind::visit(ObjCMethodPrototypeAST *ast)
{
    (void) ast;
    CPP_CHECK(!"unreachable");
    return false;
}

ObjCMethod *Bind::objCMethodPrototype(ObjCMethodPrototypeAST *ast)
{
    if (! ast)
        return nullptr;

    // int method_type_token = ast->method_type_token;
    FullySpecifiedType returnType = this->objCTypeName(ast->type_name);
    const Name *selector = this->name(ast->selector);

    const int sourceLocation = location(ast->selector, ast->firstToken());
    ObjCMethod *method = control()->newObjCMethod(sourceLocation, selector);
    // ### set the offsets
    method->setReturnType(returnType);
    if (isObjCClassMethod(tokenKind(ast->method_type_token)))
        method->setStorage(Symbol::Static);
    method->setVisibility(_objcVisibility);
    ast->symbol = method;

    Scope *previousScope = switchScope(method);
    for (ObjCMessageArgumentDeclarationListAST *it = ast->argument_list; it; it = it->next) {
        this->objCMessageArgumentDeclaration(it->value, method);
    }
    (void) switchScope(previousScope);

    if (ast->dot_dot_dot_token)
        method->setVariadic(true);

    FullySpecifiedType specifiers;
    for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
        specifiers = this->specifier(it->value, specifiers);
    }
    //setDeclSpecifiers(method, specifiers);

    return method;
}

bool Bind::visit(ObjCSynthesizedPropertyAST *ast)
{
    (void) ast;
    CPP_CHECK(!"unreachable");
    return false;
}

void Bind::objCSynthesizedProperty(ObjCSynthesizedPropertyAST *ast)
{
    if (! ast)
        return;

    // int property_identifier_token = ast->property_identifier_token;
    // int equals_token = ast->equals_token;
    // int alias_identifier_token = ast->alias_identifier_token;
}

bool Bind::visit(LambdaIntroducerAST *ast)
{
    (void) ast;
    CPP_CHECK(!"unreachable");
    return false;
}

void Bind::lambdaIntroducer(LambdaIntroducerAST *ast)
{
    if (! ast)
        return;

    // int lbracket_token = ast->lbracket_token;
    this->lambdaCapture(ast->lambda_capture);
    // int rbracket_token = ast->rbracket_token;
}

bool Bind::visit(LambdaCaptureAST *ast)
{
    (void) ast;
    CPP_CHECK(!"unreachable");
    return false;
}

void Bind::lambdaCapture(LambdaCaptureAST *ast)
{
    if (! ast)
        return;

    // int default_capture_token = ast->default_capture_token;
    for (CaptureListAST *it = ast->capture_list; it; it = it->next) {
        this->capture(it->value);
    }
}

bool Bind::visit(CaptureAST *ast)
{
    (void) ast;
    CPP_CHECK(!"unreachable");
    return false;
}

void Bind::capture(CaptureAST *ast)
{
    if (! ast)
        return;

    name(ast->identifier);
}

bool Bind::visit(LambdaDeclaratorAST *ast)
{
    (void) ast;
    CPP_CHECK(!"unreachable");
    return false;
}

Function *Bind::lambdaDeclarator(LambdaDeclaratorAST *ast)
{
    if (! ast)
        return nullptr;

    Function *fun = control()->newFunction(0, nullptr);
    fun->setStartOffset(tokenAt(ast->firstToken()).utf16charsBegin());
    fun->setEndOffset(tokenAt(ast->lastToken() - 1).utf16charsEnd());

    FullySpecifiedType type;
    if (ast->trailing_return_type)
        type = this->trailingReturnType(ast->trailing_return_type, type);
    ast->symbol = fun;

    // int lparen_token = ast->lparen_token;
    this->parameterDeclarationClause(ast->parameter_declaration_clause, ast->lparen_token, fun);
    // int rparen_token = ast->rparen_token;
    for (SpecifierListAST *it = ast->attributes; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    // int mutable_token = ast->mutable_token;
    type = this->exceptionSpecification(ast->exception_specification, type);

    if (!type.isValid())
        type.setType(control()->voidType());
    fun->setReturnType(type);
    return fun;
}

bool Bind::visit(TrailingReturnTypeAST *ast)
{
    (void) ast;
    CPP_CHECK(!"unreachable");
    return false;
}

FullySpecifiedType Bind::trailingReturnType(TrailingReturnTypeAST *ast, const FullySpecifiedType &init)
{
    FullySpecifiedType type = init;

    if (! ast)
        return type;

    // int arrow_token = ast->arrow_token;
    for (SpecifierListAST *it = ast->attributes; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    DeclaratorIdAST *declaratorId = nullptr;
    type = this->declarator(ast->declarator, type, &declaratorId);
    return type;
}

const StringLiteral *Bind::asStringLiteral(const AST *ast)
{
    CPP_ASSERT(ast, return nullptr);
    const int firstToken = ast->firstToken();
    const int lastToken = ast->lastToken();
    std::string buffer;
    for (int index = firstToken; index != lastToken; ++index) {
        const Token &tk = tokenAt(index);
        if (index != firstToken && (tk.whitespace() || tk.newline()))
            buffer += ' ';
        buffer += tk.spell();
    }
    return control()->stringLiteral(buffer.c_str(), int(buffer.size()));
}

// StatementAST
bool Bind::visit(QtMemberDeclarationAST *ast)
{
    const Name *name = nullptr;

    if (tokenKind(ast->q_token) == T_Q_D)
        name = control()->identifier("d");
    else
        name = control()->identifier("q");

    FullySpecifiedType declTy = this->expression(ast->type_id);

    if (tokenKind(ast->q_token) == T_Q_D) {
        if (NamedType *namedTy = declTy->asNamedType()) {
            if (const Identifier *nameId = namedTy->name()->asNameId()) {
                std::string privateClass;
                privateClass += nameId->identifier()->chars();
                privateClass += "Private";

                const Name *privName = control()->identifier(privateClass.c_str(), int(privateClass.size()));
                declTy.setType(control()->namedType(privName));
            }
        }
    }

    Declaration *symbol = control()->newDeclaration(/*generated*/ 0, name);
    symbol->setType(control()->pointerType(declTy));

    _scope->addMember(symbol);
    return false;
}

bool Bind::visit(CaseStatementAST *ast)
{
    ExpressionTy expression = this->expression(ast->expression);
    this->statement(ast->statement);
    return false;
}

bool Bind::visit(CompoundStatementAST *ast)
{
    Block *block = control()->newBlock(ast->firstToken());
    int startScopeToken = ast->lbrace_token ? ast->lbrace_token : ast->firstToken();
    block->setStartOffset(tokenAt(startScopeToken).utf16charsEnd());
    block->setEndOffset(tokenAt(ast->lastToken() - 1).utf16charsEnd());
    ast->symbol = block;
    _scope->addMember(block);
    Scope *previousScope = switchScope(block);
    for (StatementListAST *it = ast->statement_list; it; it = it->next) {
        this->statement(it->value);
    }
    (void) switchScope(previousScope);
    return false;
}

bool Bind::visit(DeclarationStatementAST *ast)
{
    this->declaration(ast->declaration);
    return false;
}

bool Bind::visit(DoStatementAST *ast)
{
    this->statement(ast->statement);
    ExpressionTy expression = this->expression(ast->expression);
    return false;
}

bool Bind::visit(ExpressionOrDeclarationStatementAST *ast)
{
    this->statement(ast->expression);
    this->statement(ast->declaration);
    return false;
}

bool Bind::visit(ExpressionStatementAST *ast)
{
    ExpressionTy expression = this->expression(ast->expression);
    // int semicolon_token = ast->semicolon_token;
    return false;
}

bool Bind::visit(ForeachStatementAST *ast)
{
    Block *block = control()->newBlock(ast->firstToken());
    const int startScopeToken = ast->lparen_token ? ast->lparen_token : ast->firstToken();
    block->setStartOffset(tokenAt(startScopeToken).utf16charsEnd());
    block->setEndOffset(tokenAt(ast->lastToken()).utf16charsBegin());
    _scope->addMember(block);
    ast->symbol = block;

    Scope *previousScope = switchScope(block);

    FullySpecifiedType type;
    for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    DeclaratorIdAST *declaratorId = nullptr;
    type = this->declarator(ast->declarator, type, &declaratorId);
    const StringLiteral *initializer = nullptr;
    if (type.isAuto() && translationUnit()->languageFeatures().cxx11Enabled) {
        ExpressionTy exprType = this->expression(ast->expression);

        ArrayType* arrayType = nullptr;
        arrayType = exprType->asArrayType();

        if (arrayType != nullptr)
            type = arrayType->elementType();
        else if (ast->expression != nullptr) {
            const StringLiteral *sl = asStringLiteral(ast->expression);
            const std::string buff = std::string("*") + sl->chars() + ".begin()";
            initializer = control()->stringLiteral(buff.c_str(), int(buff.size()));
        }
    }

    if (declaratorId && declaratorId->name) {
        int sourceLocation = location(declaratorId->name, ast->firstToken());
        Declaration *decl = control()->newDeclaration(sourceLocation, declaratorId->name->name);
        decl->setType(type);
        decl->setInitializer(initializer);
        block->addMember(decl);
    }

    /*ExpressionTy initializer =*/ this->expression(ast->initializer);
    /*ExpressionTy expression =*/ this->expression(ast->expression);
    this->statement(ast->statement);
    (void) switchScope(previousScope);
    return false;
}

bool Bind::visit(RangeBasedForStatementAST *ast)
{
    Block *block = control()->newBlock(ast->firstToken());
    const int startScopeToken = ast->lparen_token ? ast->lparen_token : ast->firstToken();
    block->setStartOffset(tokenAt(startScopeToken).utf16charsEnd());
    block->setEndOffset(tokenAt(ast->lastToken()).utf16charsBegin());
    _scope->addMember(block);
    ast->symbol = block;

    Scope *previousScope = switchScope(block);

    FullySpecifiedType type;
    for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    DeclaratorIdAST *declaratorId = nullptr;
    type = this->declarator(ast->declarator, type, &declaratorId);
    const StringLiteral *initializer = nullptr;
    if (type.isAuto() && translationUnit()->languageFeatures().cxx11Enabled) {
        ExpressionTy exprType = this->expression(ast->expression);

        ArrayType* arrayType = nullptr;
        arrayType = exprType->asArrayType();

        if (arrayType != nullptr)
            type = arrayType->elementType();
        else if (ast->expression != nullptr) {
            const StringLiteral *sl = asStringLiteral(ast->expression);
            const std::string buff = std::string("*") + sl->chars() + ".begin()";
            initializer = control()->stringLiteral(buff.c_str(), int(buff.size()));
        }
    }

    if (declaratorId && declaratorId->name) {
        int sourceLocation = location(declaratorId->name, ast->firstToken());
        Declaration *decl = control()->newDeclaration(sourceLocation, declaratorId->name->name);
        decl->setType(type);
        decl->setInitializer(initializer);
        block->addMember(decl);
    }

    /*ExpressionTy expression =*/ this->expression(ast->expression);
    this->statement(ast->statement);
    (void) switchScope(previousScope);
    return false;
}

bool Bind::visit(ForStatementAST *ast)
{
    Block *block = control()->newBlock(ast->firstToken());
    const int startScopeToken = ast->lparen_token ? ast->lparen_token : ast->firstToken();
    block->setStartOffset(tokenAt(startScopeToken).utf16charsEnd());
    block->setEndOffset(tokenAt(ast->lastToken()).utf16charsBegin());
    _scope->addMember(block);
    ast->symbol = block;

    Scope *previousScope = switchScope(block);
    this->statement(ast->initializer);
    /*ExpressionTy condition =*/ this->expression(ast->condition);
    // int semicolon_token = ast->semicolon_token;
    /*ExpressionTy expression =*/ this->expression(ast->expression);
    // int rparen_token = ast->rparen_token;
    this->statement(ast->statement);
    (void) switchScope(previousScope);
    return false;
}

bool Bind::visit(IfStatementAST *ast)
{
    Block *block = control()->newBlock(ast->firstToken());
    const int startScopeToken = ast->lparen_token ? ast->lparen_token : ast->firstToken();
    block->setStartOffset(tokenAt(startScopeToken).utf16charsEnd());
    block->setEndOffset(tokenAt(ast->lastToken()).utf16charsBegin());
    _scope->addMember(block);
    ast->symbol = block;

    Scope *previousScope = switchScope(block);
    /*ExpressionTy condition =*/ this->expression(ast->condition);
    this->statement(ast->statement);
    this->statement(ast->else_statement);
    (void) switchScope(previousScope);
    return false;
}

bool Bind::visit(LabeledStatementAST *ast)
{
    // int label_token = ast->label_token;
    // int colon_token = ast->colon_token;
    this->statement(ast->statement);
    return false;
}

bool Bind::visit(BreakStatementAST *ast)
{
    (void) ast;
    // int break_token = ast->break_token;
    // int semicolon_token = ast->semicolon_token;
    return false;
}

bool Bind::visit(ContinueStatementAST *ast)
{
    (void) ast;
    // int continue_token = ast->continue_token;
    // int semicolon_token = ast->semicolon_token;
    return false;
}

bool Bind::visit(GotoStatementAST *ast)
{
    (void) ast;
    // int goto_token = ast->goto_token;
    // int identifier_token = ast->identifier_token;
    // int semicolon_token = ast->semicolon_token;
    return false;
}

bool Bind::visit(ReturnStatementAST *ast)
{
    ExpressionTy expression = this->expression(ast->expression);
    return false;
}

bool Bind::visit(SwitchStatementAST *ast)
{
    Block *block = control()->newBlock(ast->firstToken());
    const int startScopeToken = ast->lparen_token ? ast->lparen_token : ast->firstToken();
    block->setStartOffset(tokenAt(startScopeToken).utf16charsEnd());
    block->setEndOffset(tokenAt(ast->lastToken()).utf16charsBegin());
    _scope->addMember(block);
    ast->symbol = block;

    Scope *previousScope = switchScope(block);
    /*ExpressionTy condition =*/ this->expression(ast->condition);
    this->statement(ast->statement);
    (void) switchScope(previousScope);
    return false;
}

bool Bind::visit(TryBlockStatementAST *ast)
{
    // int try_token = ast->try_token;
    this->statement(ast->statement);
    for (CatchClauseListAST *it = ast->catch_clause_list; it; it = it->next) {
        this->statement(it->value);
    }
    return false;
}

bool Bind::visit(CatchClauseAST *ast)
{
    Block *block = control()->newBlock(ast->firstToken());
    const int startScopeToken = ast->lparen_token ? ast->lparen_token : ast->firstToken();
    block->setStartOffset(tokenAt(startScopeToken).utf16charsEnd());
    block->setEndOffset(tokenAt(ast->lastToken()).utf16charsBegin());
    _scope->addMember(block);
    ast->symbol = block;

    Scope *previousScope = switchScope(block);
    this->declaration(ast->exception_declaration);
    // int rparen_token = ast->rparen_token;
    this->statement(ast->statement);
    (void) switchScope(previousScope);
    return false;
}

bool Bind::visit(WhileStatementAST *ast)
{
    Block *block = control()->newBlock(ast->firstToken());
    const int startScopeToken = ast->lparen_token ? ast->lparen_token : ast->firstToken();
    block->setStartOffset(tokenAt(startScopeToken).utf16charsEnd());
    block->setEndOffset(tokenAt(ast->lastToken()).utf16charsBegin());
    _scope->addMember(block);
    ast->symbol = block;

    Scope *previousScope = switchScope(block);
    /*ExpressionTy condition =*/ this->expression(ast->condition);
    this->statement(ast->statement);
    (void) switchScope(previousScope);
    return false;
}

bool Bind::visit(ObjCFastEnumerationAST *ast)
{
    Block *block = control()->newBlock(ast->firstToken());
    const int startScopeToken = ast->lparen_token ? ast->lparen_token : ast->firstToken();
    block->setStartOffset(tokenAt(startScopeToken).utf16charsEnd());
    block->setEndOffset(tokenAt(ast->lastToken()).utf16charsBegin());
    _scope->addMember(block);
    ast->symbol = block;

    Scope *previousScope = switchScope(block);
    FullySpecifiedType type;
    for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    DeclaratorIdAST *declaratorId = nullptr;
    type = this->declarator(ast->declarator, type, &declaratorId);

    if (declaratorId && declaratorId->name) {
        int sourceLocation = location(declaratorId->name, ast->firstToken());
        Declaration *decl = control()->newDeclaration(sourceLocation, declaratorId->name->name);
        decl->setType(type);
        block->addMember(decl);
    }

    /*ExpressionTy initializer =*/ this->expression(ast->initializer);
    /*ExpressionTy fast_enumeratable_expression =*/ this->expression(ast->fast_enumeratable_expression);
    this->statement(ast->statement);
    (void) switchScope(previousScope);
    return false;
}

bool Bind::visit(ObjCSynchronizedStatementAST *ast)
{
    // int synchronized_token = ast->synchronized_token;
    // int lparen_token = ast->lparen_token;
    ExpressionTy synchronized_object = this->expression(ast->synchronized_object);
    // int rparen_token = ast->rparen_token;
    this->statement(ast->statement);
    return false;
}


// ExpressionAST
bool Bind::visit(IdExpressionAST *ast)
{
    /*const Name *name =*/ this->name(ast->name);
    return false;
}

bool Bind::visit(CompoundExpressionAST *ast)
{
    // int lparen_token = ast->lparen_token;
    this->statement(ast->statement);
    // int rparen_token = ast->rparen_token;
    return false;
}

bool Bind::visit(CompoundLiteralAST *ast)
{
    // int lparen_token = ast->lparen_token;
    ExpressionTy type_id = this->expression(ast->type_id);
    // int rparen_token = ast->rparen_token;
    ExpressionTy initializer = this->expression(ast->initializer);
    return false;
}

bool Bind::visit(QtMethodAST *ast)
{
    // int method_token = ast->method_token;
    // int lparen_token = ast->lparen_token;
    FullySpecifiedType type;
    DeclaratorIdAST *declaratorId = nullptr;
    type = this->declarator(ast->declarator, type, &declaratorId);
    // int rparen_token = ast->rparen_token;
    return false;
}

bool Bind::visit(BinaryExpressionAST *ast)
{
    ExpressionTy left_expression = this->expression(ast->left_expression);
    // int binary_op_token = ast->binary_op_token;
    ExpressionTy right_expression = this->expression(ast->right_expression);
    return false;
}

bool Bind::visit(CastExpressionAST *ast)
{
    // int lparen_token = ast->lparen_token;
    ExpressionTy type_id = this->expression(ast->type_id);
    // int rparen_token = ast->rparen_token;
    ExpressionTy expression = this->expression(ast->expression);
    return false;
}

bool Bind::visit(ConditionAST *ast)
{
    FullySpecifiedType type;
    for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    DeclaratorIdAST *declaratorId = nullptr;
    type = this->declarator(ast->declarator, type, &declaratorId);

    if (declaratorId && declaratorId->name) {
        int sourceLocation = location(declaratorId->name, ast->firstToken());
        Declaration *decl = control()->newDeclaration(sourceLocation, declaratorId->name->name);
        decl->setType(type);

        if (type.isAuto() && translationUnit()->languageFeatures().cxx11Enabled)
            decl->setInitializer(asStringLiteral(ast->declarator->initializer));

        _scope->addMember(decl);
    }

    return false;
}

bool Bind::visit(ConditionalExpressionAST *ast)
{
    ExpressionTy condition = this->expression(ast->condition);
    // int question_token = ast->question_token;
    ExpressionTy left_expression = this->expression(ast->left_expression);
    // int colon_token = ast->colon_token;
    ExpressionTy right_expression = this->expression(ast->right_expression);
    return false;
}

bool Bind::visit(CppCastExpressionAST *ast)
{
    // int cast_token = ast->cast_token;
    // int less_token = ast->less_token;
    ExpressionTy type_id = this->expression(ast->type_id);
    // int greater_token = ast->greater_token;
    // int lparen_token = ast->lparen_token;
    ExpressionTy expression = this->expression(ast->expression);
    // int rparen_token = ast->rparen_token;
    return false;
}

bool Bind::visit(DeleteExpressionAST *ast)
{
    // int scope_token = ast->scope_token;
    // int delete_token = ast->delete_token;
    // int lbracket_token = ast->lbracket_token;
    // int rbracket_token = ast->rbracket_token;
    ExpressionTy expression = this->expression(ast->expression);
    return false;
}

bool Bind::visit(ArrayInitializerAST *ast)
{
    // int lbrace_token = ast->lbrace_token;
    for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
        ExpressionTy value = this->expression(it->value);
    }
    // int rbrace_token = ast->rbrace_token;
    return false;
}

bool Bind::visit(NewExpressionAST *ast)
{
    // int scope_token = ast->scope_token;
    // int new_token = ast->new_token;
    this->newPlacement(ast->new_placement);
    // int lparen_token = ast->lparen_token;
    ExpressionTy type_id = this->expression(ast->type_id);
    // int rparen_token = ast->rparen_token;
    this->newTypeId(ast->new_type_id);
    this->expression(ast->new_initializer);
    return false;
}

bool Bind::visit(TypeidExpressionAST *ast)
{
    // int typeid_token = ast->typeid_token;
    // int lparen_token = ast->lparen_token;
    ExpressionTy expression = this->expression(ast->expression);
    // int rparen_token = ast->rparen_token;
    return false;
}

bool Bind::visit(TypenameCallExpressionAST *ast)
{
    // int typename_token = ast->typename_token;
    /*const Name *name =*/ this->name(ast->name);
    this->expression(ast->expression);
    return false;
}

bool Bind::visit(TypeConstructorCallAST *ast)
{
    FullySpecifiedType type;
    for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    this->expression(ast->expression);
    return false;
}

bool Bind::visit(SizeofExpressionAST *ast)
{
    // int sizeof_token = ast->sizeof_token;
    // int dot_dot_dot_token = ast->dot_dot_dot_token;
    // int lparen_token = ast->lparen_token;
    ExpressionTy expression = this->expression(ast->expression);
    // int rparen_token = ast->rparen_token;
    return false;
}

bool Bind::visit(PointerLiteralAST *ast)
{
    (void) ast;
    // int literal_token = ast->literal_token;
    return false;
}

bool Bind::visit(NumericLiteralAST *ast)
{
    (void) ast;
    // int literal_token = ast->literal_token;
    return false;
}

bool Bind::visit(BoolLiteralAST *ast)
{
    (void) ast;
    // int literal_token = ast->literal_token;
    return false;
}

bool Bind::visit(ThisExpressionAST *ast)
{
    (void) ast;
    // int this_token = ast->this_token;
    return false;
}

bool Bind::visit(NestedExpressionAST *ast)
{
    // int lparen_token = ast->lparen_token;
    ExpressionTy expression = this->expression(ast->expression);
    // int rparen_token = ast->rparen_token;
    return false;
}

bool Bind::visit(StringLiteralAST *ast)
{
    // int literal_token = ast->literal_token;
    ExpressionTy next = this->expression(ast->next);
    return false;
}

bool Bind::visit(ThrowExpressionAST *ast)
{
    // int throw_token = ast->throw_token;
    ExpressionTy expression = this->expression(ast->expression);
    return false;
}

bool Bind::visit(TypeIdAST *ast)
{
    FullySpecifiedType type;
    for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    DeclaratorIdAST *declaratorId = nullptr;
    type = this->declarator(ast->declarator, type, &declaratorId);
    _expression = type;
    return false;
}

bool Bind::visit(UnaryExpressionAST *ast)
{
    // int unary_op_token = ast->unary_op_token;
    ExpressionTy expression = this->expression(ast->expression);
    return false;
}

bool Bind::visit(ObjCMessageExpressionAST *ast)
{
    // int lbracket_token = ast->lbracket_token;
    /*ExpressionTy receiver_expression =*/ this->expression(ast->receiver_expression);
    /*const Name *selector =*/ this->name(ast->selector);
    for (ObjCMessageArgumentListAST *it = ast->argument_list; it; it = it->next) {
        this->objCMessageArgument(it->value);
    }
    // int rbracket_token = ast->rbracket_token;
    return false;
}

bool Bind::visit(ObjCProtocolExpressionAST *ast)
{
    (void) ast;
    // int protocol_token = ast->protocol_token;
    // int lparen_token = ast->lparen_token;
    // int identifier_token = ast->identifier_token;
    // int rparen_token = ast->rparen_token;
    return false;
}

bool Bind::visit(ObjCEncodeExpressionAST *ast)
{
    // int encode_token = ast->encode_token;
    FullySpecifiedType type = this->objCTypeName(ast->type_name);
    return false;
}

bool Bind::visit(ObjCSelectorExpressionAST *ast)
{
    // int selector_token = ast->selector_token;
    // int lparen_token = ast->lparen_token;
    /*const Name *selector =*/ this->name(ast->selector);
    // int rparen_token = ast->rparen_token;
    return false;
}

bool Bind::visit(LambdaExpressionAST *ast)
{
    this->lambdaIntroducer(ast->lambda_introducer);
    if (Function *function = this->lambdaDeclarator(ast->lambda_declarator)) {
        _scope->addMember(function);
        Scope *previousScope = switchScope(function);
        this->statement(ast->statement);
        (void) switchScope(previousScope);
    } else {
        this->statement(ast->statement);
    }

    return false;
}

bool Bind::visit(BracedInitializerAST *ast)
{
    // int lbrace_token = ast->lbrace_token;
    for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
        /*ExpressionTy value =*/ this->expression(it->value);
    }
    // int comma_token = ast->comma_token;
    // int rbrace_token = ast->rbrace_token;
    return false;
}

static int methodKeyForInvokableToken(int kind)
{
    if (kind == T_Q_SIGNAL)
        return Function::SignalMethod;
    else if (kind == T_Q_SLOT)
        return Function::SlotMethod;
    else if (kind == T_Q_INVOKABLE)
        return Function::InvokableMethod;

    return Function::NormalMethod;
}

// DeclarationAST
bool Bind::visit(SimpleDeclarationAST *ast)
{
    int methodKey = _methodKey;
    if (ast->qt_invokable_token)
        methodKey = methodKeyForInvokableToken(tokenKind(ast->qt_invokable_token));

    // int qt_invokable_token = ast->qt_invokable_token;
    FullySpecifiedType type;
    for (SpecifierListAST *it = ast->decl_specifier_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }

    List<Symbol *> **symbolTail = &ast->symbols;

    if (! ast->declarator_list) {
        ElaboratedTypeSpecifierAST *elabTypeSpec = nullptr;
        for (SpecifierListAST *it = ast->decl_specifier_list; ! elabTypeSpec && it; it = it->next)
            elabTypeSpec = it->value->asElaboratedTypeSpecifier();

        if (elabTypeSpec && tokenKind(elabTypeSpec->classkey_token) != T_TYPENAME) {
            int sourceLocation = elabTypeSpec->firstToken();
            const Name *name = nullptr;
            if (elabTypeSpec->name) {
                sourceLocation = location(elabTypeSpec->name, sourceLocation);
                name = elabTypeSpec->name->name;
            }

            ensureValidClassName(&name, sourceLocation);

            ForwardClassDeclaration *decl = control()->newForwardClassDeclaration(sourceLocation, name);
            setDeclSpecifiers(decl, type);
            _scope->addMember(decl);

            *symbolTail = new (translationUnit()->memoryPool()) List<Symbol *>(decl);
            symbolTail = &(*symbolTail)->next;
        }
    }

    for (DeclaratorListAST *it = ast->declarator_list; it; it = it->next) {
        DeclaratorIdAST *declaratorId = nullptr;
        FullySpecifiedType declTy = this->declarator(it->value, type, &declaratorId);

        const Name *declName = nullptr;
        int sourceLocation = location(it->value, ast->firstToken());
        if (declaratorId && declaratorId->name)
            declName = declaratorId->name->name;

        Declaration *decl = control()->newDeclaration(sourceLocation, declName);
        decl->setType(declTy);
        setDeclSpecifiers(decl, type);

        if (Function *fun = decl->type()->asFunctionType()) {
            fun->setEnclosingScope(_scope);
            fun->setSourceLocation(sourceLocation, translationUnit());

            setDeclSpecifiers(fun, type);
            if (declaratorId && declaratorId->name)
                fun->setName(declaratorId->name->name); // update the function name
        }
        else if (declTy.isAuto()) {
            const ExpressionAST *initializer = it->value->initializer;
            if (!initializer && declaratorId)
                translationUnit()->error(location(declaratorId->name, ast->firstToken()), "auto-initialized variable must have an initializer");
            else if (initializer)
                decl->setInitializer(asStringLiteral(initializer));
        }

        if (_scope->isClass()) {
            decl->setVisibility(_visibility);

            if (Function *funTy = decl->type()->asFunctionType()) {
                funTy->setMethodKey(methodKey);

                bool pureVirtualInit = it->value->equal_token
                        && it->value->initializer
                        && it->value->initializer->asNumericLiteral();
                if (funTy->isVirtual() && pureVirtualInit)
                    funTy->setPureVirtual(true);
            }
        }

        _scope->addMember(decl);

        *symbolTail = new (translationUnit()->memoryPool()) List<Symbol *>(decl);
        symbolTail = &(*symbolTail)->next;
    }
    return false;
}

bool Bind::visit(EmptyDeclarationAST *ast)
{
    (void) ast;
    int semicolon_token = ast->semicolon_token;
    if (_scope && (_scope->isClass() || _scope->isNamespace())) {
        const Token &tk = tokenAt(semicolon_token);

        if (! tk.generated())
            translationUnit()->warning(semicolon_token, "extra `;'");
    }
    return false;
}

bool Bind::visit(AccessDeclarationAST *ast)
{
    const int accessSpecifier = tokenKind(ast->access_specifier_token);
    _visibility = visibilityForAccessSpecifier(accessSpecifier);

    if (ast->slots_token)
        _methodKey = Function::SlotMethod;
    else if (accessSpecifier == T_Q_SIGNALS)
        _methodKey = Function::SignalMethod;
    else
        _methodKey = Function::NormalMethod;

    return false;
}

bool Bind::visit(QtObjectTagAST *ast)
{
    (void) ast;
    // int q_object_token = ast->q_object_token;
    return false;
}

bool Bind::visit(QtPrivateSlotAST *ast)
{
    // int q_private_slot_token = ast->q_private_slot_token;
    // int lparen_token = ast->lparen_token;
    // int dptr_token = ast->dptr_token;
    // int dptr_lparen_token = ast->dptr_lparen_token;
    // int dptr_rparen_token = ast->dptr_rparen_token;
    // int comma_token = ast->comma_token;
    FullySpecifiedType type;
    for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    DeclaratorIdAST *declaratorId = nullptr;
    type = this->declarator(ast->declarator, type, &declaratorId);
    // int rparen_token = ast->rparen_token;
    return false;
}

static void qtPropertyAttribute(TranslationUnit *unit, ExpressionAST *expression,
                                int *flags,
                                QtPropertyDeclaration::Flag flag,
                                QtPropertyDeclaration::Flag function)
{
    if (!expression)
        return;
    *flags &= ~function & ~flag;
    if (BoolLiteralAST *boollit = expression->asBoolLiteral()) {
        const int kind = unit->tokenAt(boollit->literal_token).kind();
        if (kind == T_TRUE)
            *flags |= flag;
    } else {
        *flags |= function;
    }
}

bool Bind::visit(QtPropertyDeclarationAST *ast)
{
    // int property_specifier_token = ast->property_specifier_token;
    // int lparen_token = ast->lparen_token;
    ExpressionTy type_id = this->expression(ast->type_id);
    const Name *property_name = this->name(ast->property_name);

    int sourceLocation = ast->firstToken();
    if (ast->property_name)
        sourceLocation = ast->property_name->firstToken();
    QtPropertyDeclaration *propertyDeclaration = control()->newQtPropertyDeclaration(sourceLocation, property_name);
    propertyDeclaration->setType(type_id);

    int flags = QtPropertyDeclaration::DesignableFlag
            | QtPropertyDeclaration::ScriptableFlag
            | QtPropertyDeclaration::StoredFlag;
    for (QtPropertyDeclarationItemListAST *it = ast->property_declaration_item_list; it; it = it->next) {
        if (!it->value || !it->value->item_name_token)
            continue;
        this->expression(it->value->expression);

        std::string name = spell(it->value->item_name_token);

        if (name == "CONSTANT") {
            flags |= QtPropertyDeclaration::ConstantFlag;
        } else if (name == "FINAL") {
            flags |= QtPropertyDeclaration::FinalFlag;
        } else if (name == "READ") {
            flags |= QtPropertyDeclaration::ReadFunction;
        } else if (name == "WRITE") {
            flags |= QtPropertyDeclaration::WriteFunction;
        } else if (name == "MEMBER") {
            flags |= QtPropertyDeclaration::MemberVariable;
        } else if (name == "RESET") {
            flags |= QtPropertyDeclaration::ResetFunction;
        } else if (name == "NOTIFY") {
            flags |= QtPropertyDeclaration::NotifyFunction;
        } else if (name == "REVISION") {
            // ### handle REVISION property
        } else if (name == "DESIGNABLE") {
            qtPropertyAttribute(translationUnit(), it->value->expression, &flags,
                                QtPropertyDeclaration::DesignableFlag, QtPropertyDeclaration::DesignableFunction);
        } else if (name == "SCRIPTABLE") {
            qtPropertyAttribute(translationUnit(), it->value->expression, &flags,
                                QtPropertyDeclaration::ScriptableFlag, QtPropertyDeclaration::ScriptableFunction);
        } else if (name == "STORED") {
            qtPropertyAttribute(translationUnit(), it->value->expression, &flags,
                                QtPropertyDeclaration::StoredFlag, QtPropertyDeclaration::StoredFunction);
        } else if (name == "USER") {
            qtPropertyAttribute(translationUnit(), it->value->expression, &flags,
                                QtPropertyDeclaration::UserFlag, QtPropertyDeclaration::UserFunction);
        }
    }
    propertyDeclaration->setFlags(flags);
    _scope->addMember(propertyDeclaration);
    // int rparen_token = ast->rparen_token;
    return false;
}

bool Bind::visit(QtEnumDeclarationAST *ast)
{
    // int enum_specifier_token = ast->enum_specifier_token;
    // int lparen_token = ast->lparen_token;
    for (NameListAST *it = ast->enumerator_list; it; it = it->next) {
        const Name *value = this->name(it->value);
        if (!value)
            continue;
        QtEnum *qtEnum = control()->newQtEnum(it->value->firstToken(), value);
        _scope->addMember(qtEnum);
    }

    // int rparen_token = ast->rparen_token;
    return false;
}

bool Bind::visit(QtFlagsDeclarationAST *ast)
{
    // int flags_specifier_token = ast->flags_specifier_token;
    // int lparen_token = ast->lparen_token;
    for (NameListAST *it = ast->flag_enums_list; it; it = it->next) {
        /*const Name *value =*/ this->name(it->value);
    }
    // int rparen_token = ast->rparen_token;
    return false;
}

bool Bind::visit(QtInterfacesDeclarationAST *ast)
{
    // int interfaces_token = ast->interfaces_token;
    // int lparen_token = ast->lparen_token;
    for (QtInterfaceNameListAST *it = ast->interface_name_list; it; it = it->next) {
        this->qtInterfaceName(it->value);
    }
    // int rparen_token = ast->rparen_token;
    return false;
}

bool Bind::visit(AliasDeclarationAST *ast)
{
    if (!ast->name)
        return false;

    const Name *name = this->name(ast->name);

    FullySpecifiedType ty = expression(ast->typeId);
    ty.setTypedef(true);

    Declaration *decl = control()->newDeclaration(ast->name->firstToken(), name);
    decl->setType(ty);
    decl->setStorage(Symbol::Typedef);
    ast->symbol = decl;
    if (_scope->isClass())
        decl->setVisibility(_visibility);
    _scope->addMember(decl);

    return false;
}

bool Bind::visit(AsmDefinitionAST *ast)
{
    (void) ast;
    // int asm_token = ast->asm_token;
    // int volatile_token = ast->volatile_token;
    // int lparen_token = ast->lparen_token;
    // int rparen_token = ast->rparen_token;
    // int semicolon_token = ast->semicolon_token;
    return false;
}

bool Bind::visit(ExceptionDeclarationAST *ast)
{
    FullySpecifiedType type;
    for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    DeclaratorIdAST *declaratorId = nullptr;
    type = this->declarator(ast->declarator, type, &declaratorId);

    const Name *argName = nullptr;
    if (declaratorId && declaratorId->name)
        argName = declaratorId->name->name;
    Argument *arg = control()->newArgument(location(declaratorId, ast->firstToken()), argName);
    arg->setType(type);
    _scope->addMember(arg);

    // int dot_dot_dot_token = ast->dot_dot_dot_token;

    return false;
}

bool Bind::visit(FunctionDefinitionAST *ast)
{
    int methodKey = _methodKey;
    if (ast->qt_invokable_token)
        methodKey = methodKeyForInvokableToken(tokenKind(ast->qt_invokable_token));

    FullySpecifiedType declSpecifiers;
    for (SpecifierListAST *it = ast->decl_specifier_list; it; it = it->next) {
        declSpecifiers = this->specifier(it->value, declSpecifiers);
    }
    DeclaratorIdAST *declaratorId = nullptr;
    FullySpecifiedType type = this->declarator(ast->declarator, declSpecifiers.qualifiedType(), &declaratorId);

    Function *fun = type->asFunctionType();
    ast->symbol = fun;

    if (!fun && ast->declarator && ast->declarator->initializer)
        if (ExpressionListParenAST *exprAst = ast->declarator->initializer->asExpressionListParen()) {
            // this could be non-expanded function like macro, because
            // for find usages we parse without expanding them
            // So we create dummy function type here for findUsages to see function body
            fun = control()->newFunction(0, nullptr);
            fun->setStartOffset(tokenAt(exprAst->firstToken()).utf16charsBegin());
            fun->setEndOffset(tokenAt(exprAst->lastToken() - 1).utf16charsEnd());

            type = fun;
            ast->symbol = fun;
        }

    if (fun) {
        setDeclSpecifiers(fun, declSpecifiers);
        fun->setEndOffset(tokenAt(ast->lastToken() - 1).utf16charsEnd());

        if (_scope->isClass()) {
            fun->setVisibility(_visibility);
            fun->setMethodKey(methodKey);
        }

        if (declaratorId && declaratorId->name) {
            fun->setSourceLocation(location(declaratorId, ast->firstToken()), translationUnit());
            fun->setName(declaratorId->name->name);
        }

        _scope->addMember(fun);
    } else
        translationUnit()->warning(ast->firstToken(), "expected a function declarator");

    this->ctorInitializer(ast->ctor_initializer, fun);

    if (fun && ! _skipFunctionBodies && ast->function_body) {
        Scope *previousScope = switchScope(fun);
        this->statement(ast->function_body);
        (void) switchScope(previousScope);
    }

    return false;
}

bool Bind::visit(LinkageBodyAST *ast)
{
    // int lbrace_token = ast->lbrace_token;
    for (DeclarationListAST *it = ast->declaration_list; it; it = it->next) {
        this->declaration(it->value);
    }
    // int rbrace_token = ast->rbrace_token;
    return false;
}

bool Bind::visit(LinkageSpecificationAST *ast)
{
    // int extern_token = ast->extern_token;
    // int extern_type_token = ast->extern_type_token;
    this->declaration(ast->declaration);
    return false;
}

bool Bind::visit(NamespaceAST *ast)
{
    // int namespace_token = ast->namespace_token;
    // int identifier_token = ast->identifier_token;
    FullySpecifiedType type;
    for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }

    int sourceLocation = ast->firstToken();
    const Name *namespaceName = nullptr;
    if (ast->identifier_token) {
        sourceLocation = ast->identifier_token;
        namespaceName = identifier(ast->identifier_token);
    }

    Namespace *ns = control()->newNamespace(sourceLocation, namespaceName);
    ns->setStartOffset(tokenAt(sourceLocation).utf16charsEnd()); // the scope starts after the namespace or the identifier token.
    ns->setEndOffset(tokenAt(ast->lastToken() - 1).utf16charsEnd());
    ns->setInline(ast->inline_token != 0);
    ast->symbol = ns;
    _scope->addMember(ns);

    Scope *previousScope = switchScope(ns);
    this->declaration(ast->linkage_body);
    (void) switchScope(previousScope);
    return false;
}

bool Bind::visit(NamespaceAliasDefinitionAST *ast)
{
    int sourceLocation = ast->firstToken();
    const Name *name = nullptr;
    if (ast->namespace_name_token) {
        sourceLocation = ast->namespace_name_token;
        name = identifier(ast->namespace_name_token);
    }

    NamespaceAlias *namespaceAlias = control()->newNamespaceAlias(sourceLocation, name);
    namespaceAlias->setNamespaceName(this->name(ast->name));
    _scope->addMember(namespaceAlias);
    return false;
}

bool Bind::visit(ParameterDeclarationAST *ast)
{
    FullySpecifiedType type;
    for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    DeclaratorIdAST *declaratorId = nullptr;
    type = this->declarator(ast->declarator, type, &declaratorId);
    // int equal_token = ast->equal_token;
    ExpressionTy expression = this->expression(ast->expression);

    const Name *argName = nullptr;
    if (declaratorId && declaratorId->name)
        argName = declaratorId->name->name;

    Argument *arg = control()->newArgument(location(declaratorId, ast->firstToken()), argName);
    arg->setType(type);

    if (ast->expression)
        arg->setInitializer(asStringLiteral(ast->expression));

    _scope->addMember(arg);

    ast->symbol = arg;
    return false;
}

bool Bind::visit(TemplateDeclarationAST *ast)
{
    Template *templ = control()->newTemplate(ast->firstToken(), nullptr);
    templ->setStartOffset(tokenAt(ast->firstToken()).utf16charsBegin());
    templ->setEndOffset(tokenAt(ast->lastToken() - 1).utf16charsEnd());
    ast->symbol = templ;
    Scope *previousScope = switchScope(templ);

    for (DeclarationListAST *it = ast->template_parameter_list; it; it = it->next) {
        this->declaration(it->value);
    }
    // int greater_token = ast->greater_token;
    this->declaration(ast->declaration);
    (void) switchScope(previousScope);

    if (Symbol *decl = templ->declaration()) {
        templ->setSourceLocation(decl->sourceLocation(), translationUnit());
        templ->setName(decl->name());
    }

    _scope->addMember(templ);
    return false;
}

bool Bind::visit(TypenameTypeParameterAST *ast)
{
    int sourceLocation = location(ast->name, ast->firstToken());
    // int classkey_token = ast->classkey_token;
    // int dot_dot_dot_token = ast->dot_dot_dot_token;
    const Name *name = this->name(ast->name);
    ExpressionTy type_id = this->expression(ast->type_id);
    CPlusPlus::Kind classKey = translationUnit()->tokenKind(ast->classkey_token);

    TypenameArgument *arg = control()->newTypenameArgument(sourceLocation, name);
    arg->setType(type_id);
    arg->setClassDeclarator(classKey == T_CLASS);
    ast->symbol = arg;
    _scope->addMember(arg);
    return false;
}

bool Bind::visit(TemplateTypeParameterAST *ast)
{
    int sourceLocation = location(ast->name, ast->firstToken());

    // int template_token = ast->template_token;
    // int less_token = ast->less_token;
    // ### process the template prototype
#if 0
    for (DeclarationListAST *it = ast->template_parameter_list; it; it = it->next) {
        this->declaration(it->value);
    }
#endif
    // int greater_token = ast->greater_token;
    // int class_token = ast->class_token;
    // int dot_dot_dot_token = ast->dot_dot_dot_token;

    const Name *name = this->name(ast->name);
    ExpressionTy type_id = this->expression(ast->type_id);

    // ### introduce TemplateTypeArgument
    TypenameArgument *arg = control()->newTypenameArgument(sourceLocation, name);
    arg->setType(type_id);
    ast->symbol = arg;
    _scope->addMember(arg);

    return false;
}

bool Bind::visit(UsingAST *ast)
{
    int sourceLocation = location(ast->name, ast->firstToken());
    const Name *name = this->name(ast->name);

    UsingDeclaration *symbol = control()->newUsingDeclaration(sourceLocation, name);
    ast->symbol = symbol;
    _scope->addMember(symbol);
    return false;
}

bool Bind::visit(UsingDirectiveAST *ast)
{
    int sourceLocation = location(ast->name, ast->firstToken());
    const Name *name = this->name(ast->name);
    UsingNamespaceDirective *symbol = control()->newUsingNamespaceDirective(sourceLocation, name);
    ast->symbol = symbol;
    _scope->addMember(symbol);
    return false;
}

bool Bind::visit(ObjCClassForwardDeclarationAST *ast)
{
    FullySpecifiedType declSpecifiers;
    for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
        declSpecifiers = this->specifier(it->value, declSpecifiers);
    }

    List<ObjCForwardClassDeclaration *> **symbolTail = &ast->symbols;

    // int class_token = ast->class_token;
    for (NameListAST *it = ast->identifier_list; it; it = it->next) {
        const Name *name = this->name(it->value);

        const int sourceLocation = location(it->value, ast->firstToken());
        ObjCForwardClassDeclaration *fwd = control()->newObjCForwardClassDeclaration(sourceLocation, name);
        setDeclSpecifiers(fwd, declSpecifiers);
        _scope->addMember(fwd);

        *symbolTail = new (translationUnit()->memoryPool()) List<ObjCForwardClassDeclaration *>(fwd);
        symbolTail = &(*symbolTail)->next;
    }

    return false;
}

int Bind::calculateScopeStart(ObjCClassDeclarationAST *ast) const
{
    if (ast->inst_vars_decl)
        if (int pos = ast->inst_vars_decl->lbrace_token)
            return tokenAt(pos).utf16charsEnd();

    if (ast->protocol_refs)
        if (int pos = ast->protocol_refs->lastToken())
            return tokenAt(pos - 1).utf16charsEnd();

    if (ast->superclass)
        if (int pos = ast->superclass->lastToken())
            return tokenAt(pos - 1).utf16charsEnd();

    if (ast->colon_token)
        return tokenAt(ast->colon_token).utf16charsEnd();

    if (ast->rparen_token)
        return tokenAt(ast->rparen_token).utf16charsEnd();

    if (ast->category_name)
        if (int pos = ast->category_name->lastToken())
            return tokenAt(pos - 1).utf16charsEnd();

    if (ast->lparen_token)
        return tokenAt(ast->lparen_token).utf16charsEnd();

    if (ast->class_name)
        if (int pos = ast->class_name->lastToken())
            return tokenAt(pos - 1).utf16charsEnd();

    return tokenAt(ast->firstToken()).utf16charsBegin();
}

bool Bind::visit(ObjCClassDeclarationAST *ast)
{
    FullySpecifiedType declSpecifiers;
    for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
        declSpecifiers = this->specifier(it->value, declSpecifiers);
    }
    const Name *class_name = this->name(ast->class_name);
    const Name *category_name = this->name(ast->category_name);

    const int sourceLocation = location(ast->class_name, ast->firstToken());
    ObjCClass *klass = control()->newObjCClass(sourceLocation, class_name);
    ast->symbol = klass;
    _scope->addMember(klass);

    klass->setStartOffset(calculateScopeStart(ast));
    klass->setEndOffset(tokenAt(ast->lastToken() - 1).utf16charsEnd());

    if (ast->interface_token)
        klass->setInterface(true);

    klass->setCategoryName(category_name);

    Scope *previousScope = switchScope(klass);

    if (const Name *superclass = this->name(ast->superclass)) {
        ObjCBaseClass *superKlass = control()->newObjCBaseClass(ast->superclass->firstToken(), superclass);
        klass->setBaseClass(superKlass);
    }

    this->objCProtocolRefs(ast->protocol_refs, klass);

    const int previousObjCVisibility = switchObjCVisibility(Function::Protected);

    this->objCInstanceVariablesDeclaration(ast->inst_vars_decl, klass);

    (void) switchObjCVisibility(Function::Public);
    for (DeclarationListAST *it = ast->member_declaration_list; it; it = it->next) {
        this->declaration(it->value);
    }

    (void) switchObjCVisibility(previousObjCVisibility);
    (void) switchScope(previousScope);
    return false;
}

bool Bind::visit(ObjCProtocolForwardDeclarationAST *ast)
{
    FullySpecifiedType declSpecifiers;
    for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
        declSpecifiers = this->specifier(it->value, declSpecifiers);
    }

    List<ObjCForwardProtocolDeclaration *> **symbolTail = &ast->symbols;

    // int class_token = ast->class_token;
    for (NameListAST *it = ast->identifier_list; it; it = it->next) {
        const Name *name = this->name(it->value);

        const int sourceLocation = location(it->value, ast->firstToken());
        ObjCForwardProtocolDeclaration *fwd = control()->newObjCForwardProtocolDeclaration(sourceLocation, name);
        setDeclSpecifiers(fwd, declSpecifiers);
        _scope->addMember(fwd);

        *symbolTail = new (translationUnit()->memoryPool()) List<ObjCForwardProtocolDeclaration *>(fwd);
        symbolTail = &(*symbolTail)->next;
    }

    return false;
}

int Bind::calculateScopeStart(ObjCProtocolDeclarationAST *ast) const
{
    if (ast->protocol_refs)
        if (int pos = ast->protocol_refs->lastToken())
            return tokenAt(pos - 1).utf16charsEnd();
    if (ast->name)
        if (int pos = ast->name->lastToken())
            return tokenAt(pos - 1).utf16charsEnd();

    return tokenAt(ast->firstToken()).utf16charsBegin();
}

bool Bind::visit(ObjCProtocolDeclarationAST *ast)
{
    FullySpecifiedType type;
    for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    // int protocol_token = ast->protocol_token;
    const Name *name = this->name(ast->name);

    const int sourceLocation = location(ast->name, ast->firstToken());
    ObjCProtocol *protocol = control()->newObjCProtocol(sourceLocation, name);
    protocol->setStartOffset(calculateScopeStart(ast));
    protocol->setEndOffset(tokenAt(ast->lastToken() - 1).utf16charsEnd());
    ast->symbol = protocol;
    _scope->addMember(protocol);

    Scope *previousScope = switchScope(protocol);
    const int previousObjCVisibility = switchObjCVisibility(Function::Public);

    this->objCProtocolRefs(ast->protocol_refs, protocol);
    for (DeclarationListAST *it = ast->member_declaration_list; it; it = it->next) {
        this->declaration(it->value);
    }

    (void) switchObjCVisibility(previousObjCVisibility);
    (void) switchScope(previousScope);
    return false;
}

bool Bind::visit(ObjCVisibilityDeclarationAST *ast)
{
    _objcVisibility = visibilityForObjCAccessSpecifier(tokenKind(ast->visibility_token));
    return false;
}

bool Bind::visit(ObjCPropertyDeclarationAST *ast)
{
    (void) ast;
    FullySpecifiedType type;
    for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    // int property_token = ast->property_token;
    // int lparen_token = ast->lparen_token;
    for (ObjCPropertyAttributeListAST *it = ast->property_attribute_list; it; it = it->next) {
        this->objCPropertyAttribute(it->value);
    }
    // int rparen_token = ast->rparen_token;
    this->declaration(ast->simple_declaration);
    // List<ObjCPropertyDeclaration *> *symbols = ast->symbols;
    return false;
}

bool Bind::visit(ObjCMethodDeclarationAST *ast)
{
    ObjCMethod *method = this->objCMethodPrototype(ast->method_prototype);

    if (! ast->function_body) {
        const Name *name = method->name();
        int sourceLocation = ast->firstToken();
        Declaration *decl = control()->newDeclaration(sourceLocation, name);
        decl->setType(method);
        _scope->addMember(decl);
    } else if (! _skipFunctionBodies && ast->function_body) {
        Scope *previousScope = switchScope(method);
        this->statement(ast->function_body);
        (void) switchScope(previousScope);
        _scope->addMember(method);
    } else if (method) {
        _scope->addMember(method);
    }

    return false;
}

bool Bind::visit(ObjCSynthesizedPropertiesDeclarationAST *ast)
{
    // int synthesized_token = ast->synthesized_token;
    for (ObjCSynthesizedPropertyListAST *it = ast->property_identifier_list; it; it = it->next) {
        this->objCSynthesizedProperty(it->value);
    }
    // int semicolon_token = ast->semicolon_token;
    return false;
}

bool Bind::visit(ObjCDynamicPropertiesDeclarationAST *ast)
{
    // int dynamic_token = ast->dynamic_token;
    for (NameListAST *it = ast->property_identifier_list; it; it = it->next) {
        /*const Name *value =*/ this->name(it->value);
    }
    // int semicolon_token = ast->semicolon_token;
    return false;
}


// NameAST
bool Bind::visit(ObjCSelectorAST *ast) // ### review
{
    std::vector<const Name *> arguments;
    bool hasArgs = false;

    for (ObjCSelectorArgumentListAST *it = ast->selector_argument_list; it; it = it->next) {
        if (const Name *selector_argument = this->objCSelectorArgument(it->value, &hasArgs))
            arguments.push_back(selector_argument);
    }

    if (! arguments.empty()) {
        _name = control()->selectorNameId(&arguments[0], int(arguments.size()), hasArgs);
        ast->name = _name;
    }

    return false;
}

bool Bind::visit(QualifiedNameAST *ast)
{
    for (NestedNameSpecifierListAST *it = ast->nested_name_specifier_list; it; it = it->next) {
        const Name *class_or_namespace_name = this->nestedNameSpecifier(it->value);
        if (_name || ast->global_scope_token)
            _name = control()->qualifiedNameId(_name, class_or_namespace_name);
        else
            _name = class_or_namespace_name;
    }

    const Name *unqualified_name = this->name(ast->unqualified_name);
    if (_name || ast->global_scope_token)
        _name = control()->qualifiedNameId(_name, unqualified_name);
    else
        _name = unqualified_name;

    ast->name = _name;
    return false;
}

bool Bind::visit(OperatorFunctionIdAST *ast)
{
    const OperatorNameId::Kind op = this->cppOperator(ast->op);
    ast->name = _name = control()->operatorNameId(op);
    return false;
}

bool Bind::visit(ConversionFunctionIdAST *ast)
{
    FullySpecifiedType type;
    for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    for (PtrOperatorListAST *it = ast->ptr_operator_list; it; it = it->next) {
        type = this->ptrOperator(it->value, type);
    }
    ast->name = _name = control()->conversionNameId(type);
    return false;
}

bool Bind::visit(AnonymousNameAST *ast)
{
    ast->name = _name = control()->anonymousNameId(ast->class_token);
    return false;
}

bool Bind::visit(SimpleNameAST *ast)
{
    const Identifier *id = identifier(ast->identifier_token);
    _name = id;
    ast->name = _name;
    return false;
}

bool Bind::visit(DestructorNameAST *ast)
{
    _name = control()->destructorNameId(name(ast->unqualified_name));
    ast->name = _name;
    return false;
}

bool Bind::visit(TemplateIdAST *ast)
{
    // collect the template parameters
    std::vector<TemplateArgument> templateArguments;
    for (ExpressionListAST *it = ast->template_argument_list; it; it = it->next) {
        ExpressionTy value = this->expression(it->value);
        if (value.isValid()) {
            templateArguments.emplace_back(value);
        } else {
            // special case for numeric values
            if (it->value->asNumericLiteral()) {
                templateArguments
                    .emplace_back(value,
                                  tokenAt(it->value->asNumericLiteral()->literal_token).number);
            } else if (it->value->asBoolLiteral()) {
                templateArguments
                    .emplace_back(value, tokenAt(it->value->asBoolLiteral()->literal_token).number);
            } else {
                // fall back to non-valid type in templateArguments
                // for ast->template_argument_list and templateArguments sizes match
                // TODO support other literals/expressions as default arguments
                templateArguments.emplace_back(value);
            }
        }
    }

    const Identifier *id = identifier(ast->identifier_token);
    const int tokenKindBeforeIdentifier(translationUnit()->tokenKind(ast->identifier_token - 1));
    const bool isSpecialization = (tokenKindBeforeIdentifier == T_CLASS ||
                                   tokenKindBeforeIdentifier == T_STRUCT);
    if (templateArguments.empty())
        _name = control()->templateNameId(id, isSpecialization);
    else
        _name = control()->templateNameId(id, isSpecialization, &templateArguments[0],
                int(templateArguments.size()));

    ast->name = _name;
    return false;
}


// SpecifierAST
bool Bind::visit(SimpleSpecifierAST *ast)
{
    switch (tokenKind(ast->specifier_token)) {
        case T_IDENTIFIER: {
                const Identifier *id = tokenAt(ast->specifier_token).identifier;
                if (id->match(control()->cpp11Override())) {
                    if (_type.isOverride())
                        translationUnit()->error(ast->specifier_token, "duplicate `override'");
                    _type.setOverride(true);
                }
                else if (id->match(control()->cpp11Final())) {
                    if (_type.isFinal())
                        translationUnit()->error(ast->specifier_token, "duplicate `final'");
                    _type.setFinal(true);
                }
            }
            break;
        case T_CONST:
            if (_type.isConst())
                translationUnit()->error(ast->specifier_token, "duplicate `%s'", spell(ast->specifier_token));
            _type.setConst(true);
            break;

        case T_VOLATILE:
            if (_type.isVolatile())
                translationUnit()->error(ast->specifier_token, "duplicate `%s'", spell(ast->specifier_token));
            _type.setVolatile(true);
            break;

        case T_FRIEND:
            if (_type.isFriend())
                translationUnit()->error(ast->specifier_token, "duplicate `%s'", spell(ast->specifier_token));
            _type.setFriend(true);
            break;

        case T_AUTO:
            if (!translationUnit()->languageFeatures().cxx11Enabled) {
                if (_type.isAuto())
                    translationUnit()->error(ast->specifier_token, "duplicate `%s'", spell(ast->specifier_token));
            }
            _type.setAuto(true);
            break;

        case T_REGISTER:
            if (_type.isRegister())
                translationUnit()->error(ast->specifier_token, "duplicate `%s'", spell(ast->specifier_token));
            _type.setRegister(true);
            break;

        case T_STATIC:
            if (_type.isStatic())
                translationUnit()->error(ast->specifier_token, "duplicate `%s'", spell(ast->specifier_token));
            _type.setStatic(true);
            break;

        case T_EXTERN:
            if (_type.isExtern())
                translationUnit()->error(ast->specifier_token, "duplicate `%s'", spell(ast->specifier_token));
            _type.setExtern(true);
            break;

        case T_MUTABLE:
            if (_type.isMutable())
                translationUnit()->error(ast->specifier_token, "duplicate `%s'", spell(ast->specifier_token));
            _type.setMutable(true);
            break;

        case T_TYPEDEF:
            if (_type.isTypedef())
                translationUnit()->error(ast->specifier_token, "duplicate `%s'", spell(ast->specifier_token));
            _type.setTypedef(true);
            break;

        case T_INLINE:
            if (_type.isInline())
                translationUnit()->error(ast->specifier_token, "duplicate `%s'", spell(ast->specifier_token));
            _type.setInline(true);
            break;

        case T_VIRTUAL:
            if (_type.isVirtual())
                translationUnit()->error(ast->specifier_token, "duplicate `%s'", spell(ast->specifier_token));
            _type.setVirtual(true);
            break;

        case T_EXPLICIT:
            if (_type.isExplicit())
                translationUnit()->error(ast->specifier_token, "duplicate `%s'", spell(ast->specifier_token));
            _type.setExplicit(true);
            break;

        case T_SIGNED:
            if (_type.isSigned())
                translationUnit()->error(ast->specifier_token, "duplicate `%s'", spell(ast->specifier_token));
            _type.setSigned(true);
            break;

        case T_UNSIGNED:
            if (_type.isUnsigned())
                translationUnit()->error(ast->specifier_token, "duplicate `%s'", spell(ast->specifier_token));
            _type.setUnsigned(true);
            break;

        case T_CHAR:
            if (_type)
                translationUnit()->error(ast->specifier_token, "duplicate data type in declaration");
            _type.setType(control()->integerType(IntegerType::Char));
            break;

        case T_CHAR16_T:
            if (_type)
                translationUnit()->error(ast->specifier_token, "duplicate data type in declaration");
            _type.setType(control()->integerType(IntegerType::Char16));
            break;

        case T_CHAR32_T:
            if (_type)
                translationUnit()->error(ast->specifier_token, "duplicate data type in declaration");
            _type.setType(control()->integerType(IntegerType::Char32));
            break;

        case T_WCHAR_T:
            if (_type)
                translationUnit()->error(ast->specifier_token, "duplicate data type in declaration");
            _type.setType(control()->integerType(IntegerType::WideChar));
            break;

        case T_BOOL:
            if (_type)
                translationUnit()->error(ast->specifier_token, "duplicate data type in declaration");
            _type.setType(control()->integerType(IntegerType::Bool));
            break;

        case T_SHORT:
            if (_type) {
                IntegerType *intType = control()->integerType(IntegerType::Int);
                if (_type.type() != intType)
                    translationUnit()->error(ast->specifier_token, "duplicate data type in declaration");
            }
            _type.setType(control()->integerType(IntegerType::Short));
            break;

        case T_INT:
            if (_type) {
                Type *tp = _type.type();
                IntegerType *shortType = control()->integerType(IntegerType::Short);
                IntegerType *longType = control()->integerType(IntegerType::Long);
                IntegerType *longLongType = control()->integerType(IntegerType::LongLong);
                if (tp == shortType || tp == longType || tp == longLongType)
                    break;
                translationUnit()->error(ast->specifier_token, "duplicate data type in declaration");
            }
            _type.setType(control()->integerType(IntegerType::Int));
            break;

        case T_LONG:
            if (_type) {
                Type *tp = _type.type();
                IntegerType *intType = control()->integerType(IntegerType::Int);
                IntegerType *longType = control()->integerType(IntegerType::Long);
                FloatType *doubleType = control()->floatType(FloatType::Double);
                if (tp == longType) {
                    _type.setType(control()->integerType(IntegerType::LongLong));
                    break;
                } else if (tp == doubleType) {
                    _type.setType(control()->floatType(FloatType::LongDouble));
                    break;
                } else if (tp != intType) {
                    translationUnit()->error(ast->specifier_token, "duplicate data type in declaration");
                }
            }
            _type.setType(control()->integerType(IntegerType::Long));
            break;

        case T_FLOAT:
            if (_type)
                translationUnit()->error(ast->specifier_token, "duplicate data type in declaration");
            _type.setType(control()->floatType(FloatType::Float));
            break;

        case T_DOUBLE:
            if (_type) {
                IntegerType *longType = control()->integerType(IntegerType::Long);
                if (_type.type() == longType) {
                    _type.setType(control()->floatType(FloatType::LongDouble));
                    break;
                }
                translationUnit()->error(ast->specifier_token, "duplicate data type in declaration");
            }
            _type.setType(control()->floatType(FloatType::Double));
            break;

        case T_VOID:
            if (_type)
                translationUnit()->error(ast->specifier_token, "duplicate data type in declaration");
            _type.setType(control()->voidType());
            break;

        default:
            break;
    } // switch
    return false;
}

bool Bind::visit(AlignmentSpecifierAST *ast)
{
    // Prevent visiting the type-id or alignment expression from changing the currently
    // calculated type:
    expression(ast->typeIdExprOrAlignmentExpr);
    return false;
}

bool Bind::visit(GnuAttributeSpecifierAST *ast)
{
    // int attribute_token = ast->attribute_token;
    // int first_lparen_token = ast->first_lparen_token;
    // int second_lparen_token = ast->second_lparen_token;
    for (GnuAttributeListAST *it = ast->attribute_list; it; it = it->next) {
        this->attribute(it->value);
    }
    // int first_rparen_token = ast->first_rparen_token;
    // int second_rparen_token = ast->second_rparen_token;
    return false;
}

bool Bind::visit(MsvcDeclspecSpecifierAST *ast)
{
    for (GnuAttributeListAST *it = ast->attribute_list; it; it = it->next) {
        this->attribute(it->value);
    }
    return false;
}

bool Bind::visit(StdAttributeSpecifierAST *ast)
{
    for (GnuAttributeListAST *it = ast->attribute_list; it; it = it->next) {
        this->attribute(it->value);
    }
    return false;
}

bool Bind::visit(TypeofSpecifierAST *ast)
{
    ExpressionTy expression = this->expression(ast->expression);
    _type = expression;
    return false;
}

bool Bind::visit(DecltypeSpecifierAST *ast)
{
    _type = this->expression(ast->expression);
    return false;
}

bool Bind::visit(ClassSpecifierAST *ast)
{
    // int classkey_token = ast->classkey_token;
    int sourceLocation = ast->firstToken();
    int startScopeOffset = tokenAt(sourceLocation).utf16charsEnd(); // at the end of the class key

    for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
        _type = this->specifier(it->value, _type);
    }

    const Name *className = this->name(ast->name);

    if (ast->name && ! ast->name->asAnonymousName()) {
        sourceLocation = location(ast->name, sourceLocation);
        startScopeOffset = tokenAt(sourceLocation).utf16charsEnd(); // at the end of the class name

        if (QualifiedNameAST *q = ast->name->asQualifiedName()) {
            if (q->unqualified_name) {
                sourceLocation = q->unqualified_name->firstToken();
                startScopeOffset = tokenAt(q->unqualified_name->lastToken() - 1).utf16charsEnd(); // at the end of the unqualified name
            }
        }

        ensureValidClassName(&className, sourceLocation);
    }

    Class *klass = control()->newClass(sourceLocation, className);
    klass->setStartOffset(startScopeOffset);
    klass->setEndOffset(tokenAt(ast->lastToken() - 1).utf16charsEnd());
    _scope->addMember(klass);

    if (_scope->isClass())
        klass->setVisibility(_visibility);

    // set the class key
    int classKey = tokenKind(ast->classkey_token);
    if (classKey == T_CLASS)
        klass->setClassKey(Class::ClassKey);
    else if (classKey == T_STRUCT)
        klass->setClassKey(Class::StructKey);
    else if (classKey == T_UNION)
        klass->setClassKey(Class::UnionKey);

    _type.setType(klass);

    Scope *previousScope = switchScope(klass);
    const int previousVisibility = switchVisibility(visibilityForClassKey(classKey));
    const int previousMethodKey = switchMethodKey(Function::NormalMethod);

    for (BaseSpecifierListAST *it = ast->base_clause_list; it; it = it->next) {
        this->baseSpecifier(it->value, ast->colon_token, klass);
    }
    // int dot_dot_dot_token = ast->dot_dot_dot_token;
    for (DeclarationListAST *it = ast->member_specifier_list; it; it = it->next) {
        this->declaration(it->value);
    }

    (void) switchMethodKey(previousMethodKey);
    (void) switchVisibility(previousVisibility);
    (void) switchScope(previousScope);

    ast->symbol = klass;
    return false;
}

bool Bind::visit(NamedTypeSpecifierAST *ast)
{
    _type.setType(control()->namedType(this->name(ast->name)));
    return false;
}

bool Bind::visit(ElaboratedTypeSpecifierAST *ast)
{
    // int classkey_token = ast->classkey_token;
    for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
        _type = this->specifier(it->value, _type);
    }
    _type.setType(control()->namedType(this->name(ast->name)));
    return false;
}

bool Bind::visit(EnumSpecifierAST *ast)
{
    int sourceLocation = location(ast->name, ast->firstToken());
    const Name *enumName = this->name(ast->name);

    Enum *e = control()->newEnum(sourceLocation, enumName);
    e->setStartOffset(tokenAt(sourceLocation).utf16charsEnd()); // at the end of the enum or identifier token.
    e->setEndOffset(tokenAt(ast->lastToken() - 1).utf16charsEnd());
    if (ast->key_token)
        e->setScoped(true);
    ast->symbol = e;
    _scope->addMember(e);

    if (_scope->isClass())
        e->setVisibility(_visibility);

    Scope *previousScope = switchScope(e);
    for (EnumeratorListAST *it = ast->enumerator_list; it; it = it->next) {
        this->enumerator(it->value, e);
    }

    (void) switchScope(previousScope);
    if (ast->name)
        _type.setType(control()->namedType(this->name(ast->name)));
    else
        _type.setType(ast->symbol);
    return false;
}

// PtrOperatorAST
bool Bind::visit(PointerToMemberAST *ast)
{
    const Name *memberName = nullptr;

    for (NestedNameSpecifierListAST *it = ast->nested_name_specifier_list; it; it = it->next) {
        const Name *class_or_namespace_name = this->nestedNameSpecifier(it->value);
        if (memberName || ast->global_scope_token)
            memberName = control()->qualifiedNameId(memberName, class_or_namespace_name);
        else
            memberName = class_or_namespace_name;
    }

    FullySpecifiedType type(control()->pointerToMemberType(memberName, _type));
    for (SpecifierListAST *it = ast->cv_qualifier_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    _type = type;
    return false;
}

bool Bind::visit(PointerAST *ast)
{
    if (_type->isReferenceType())
        translationUnit()->error(ast->firstToken(), "cannot declare pointer to a reference");

    FullySpecifiedType type(control()->pointerType(_type));
    for (SpecifierListAST *it = ast->cv_qualifier_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }
    _type = type;
    return false;
}

bool Bind::visit(ReferenceAST *ast)
{
    const bool rvalueRef = (tokenKind(ast->reference_token) == T_AMPER_AMPER);

    if (_type->isReferenceType())
        translationUnit()->error(ast->firstToken(), "cannot declare reference to a reference");

    FullySpecifiedType type(control()->referenceType(_type, rvalueRef));
    _type = type;
    return false;
}


// PostfixAST
bool Bind::visit(CallAST *ast)
{
    /*ExpressionTy base_expression =*/ this->expression(ast->base_expression);
    // int lparen_token = ast->lparen_token;
    for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
        /*ExpressionTy value =*/ this->expression(it->value);
    }
    // int rparen_token = ast->rparen_token;
    return false;
}

bool Bind::visit(ArrayAccessAST *ast)
{
    /*ExpressionTy base_expression =*/ this->expression(ast->base_expression);
    // int lbracket_token = ast->lbracket_token;
    /*ExpressionTy expression =*/ this->expression(ast->expression);
    // int rbracket_token = ast->rbracket_token;
    return false;
}

bool Bind::visit(PostIncrDecrAST *ast)
{
    ExpressionTy base_expression = this->expression(ast->base_expression);
    // int incr_decr_token = ast->incr_decr_token;
    return false;
}

bool Bind::visit(MemberAccessAST *ast)
{
    ExpressionTy base_expression = this->expression(ast->base_expression);
    // int access_token = ast->access_token;
    // int template_token = ast->template_token;
    /*const Name *member_name =*/ this->name(ast->member_name);
    return false;
}


// CoreDeclaratorAST
bool Bind::visit(DeclaratorIdAST *ast)
{
    /*const Name *name =*/ this->name(ast->name);
    *_declaratorId = ast;
    return false;
}

bool Bind::visit(NestedDeclaratorAST *ast)
{
    _type = this->declarator(ast->declarator, _type, _declaratorId);
    return false;
}


// PostfixDeclaratorAST
bool Bind::visit(FunctionDeclaratorAST *ast)
{
    Function *fun = control()->newFunction(0, nullptr);
    fun->setStartOffset(tokenAt(ast->firstToken()).utf16charsBegin());
    fun->setEndOffset(tokenAt(ast->lastToken() - 1).utf16charsEnd());
    if (ast->trailing_return_type)
        _type = this->trailingReturnType(ast->trailing_return_type, _type);
    fun->setReturnType(_type);

    // int lparen_token = ast->lparen_token;
    this->parameterDeclarationClause(ast->parameter_declaration_clause, ast->lparen_token, fun);
    // int rparen_token = ast->rparen_token;
    FullySpecifiedType type(fun);
    for (SpecifierListAST *it = ast->cv_qualifier_list; it; it = it->next) {
        type = this->specifier(it->value, type);
    }

    // propagate the cv-qualifiers
    fun->setConst(type.isConst());
    fun->setVolatile(type.isVolatile());
    fun->setOverride(type.isOverride());
    fun->setFinal(type.isFinal());

    // propagate ref-qualifier
    if (ast->ref_qualifier_token) {
        const Kind kind = tokenAt(ast->ref_qualifier_token).kind();
        CPP_CHECK(kind == T_AMPER || kind == T_AMPER_AMPER); // & or && are only allowed
        fun->setRefQualifier(kind == T_AMPER ? Function::LvalueRefQualifier :
                                               Function::RvalueRefQualifier);
    }

    // propagate exception spec
    this->exceptionSpecification(ast->exception_specification, type);
    if (ExceptionSpecificationAST *spec = ast->exception_specification)
        fun->setExceptionSpecification(asStringLiteral(spec));

    if (ast->as_cpp_initializer != nullptr) {
        fun->setAmbiguous(true);
        /*ExpressionTy as_cpp_initializer =*/ this->expression(ast->as_cpp_initializer);
    }
    ast->symbol = fun;
    _type = type;
    return false;
}

bool Bind::visit(ArrayDeclaratorAST *ast)
{
    ExpressionTy expression = this->expression(ast->expression);
    FullySpecifiedType type(control()->arrayType(_type));
    _type = type;
    return false;
}

void Bind::ensureValidClassName(const Name **name, int sourceLocation)
{
    if (!*name)
        return;

    const QualifiedNameId *qName = (*name)->asQualifiedNameId();
    const Name *uqName = qName ? qName->name() : *name;

    if (!uqName->isNameId() && !uqName->isTemplateNameId()) {
        translationUnit()->error(sourceLocation, "expected a class-name");

        *name = uqName->identifier();
        if (qName)
            *name = control()->qualifiedNameId(qName->base(), *name);
    }
}

int Bind::visibilityForAccessSpecifier(int tokenKind)
{
    switch (tokenKind) {
    case T_PUBLIC:
        return Symbol::Public;
    case T_PROTECTED:
        return Symbol::Protected;
    case T_PRIVATE:
        return Symbol::Private;
    case T_Q_SIGNALS:
        return Symbol::Protected;
    default:
        return Symbol::Public;
    }
}

int Bind::visibilityForClassKey(int tokenKind)
{
    switch (tokenKind) {
    case T_CLASS:
        return Symbol::Private;
    case T_STRUCT:
    case T_UNION:
        return Symbol::Public;
    default:
        return Symbol::Public;
    }
}

int Bind::visibilityForObjCAccessSpecifier(int tokenKind)
{
    switch (tokenKind) {
    case T_AT_PUBLIC:
        return Symbol::Public;
    case T_AT_PROTECTED:
        return Symbol::Protected;
    case T_AT_PRIVATE:
        return Symbol::Private;
    case T_AT_PACKAGE:
        return Symbol::Package;
    default:
        return Symbol::Protected;
    }
}

bool Bind::isObjCClassMethod(int tokenKind)
{
    switch (tokenKind) {
    case T_PLUS:
        return true;
    case T_MINUS:
    default:
        return false;
    }
}
