/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/
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

#include "CheckDeclaration.h"
#include "Semantic.h"
#include "AST.h"
#include "TranslationUnit.h"
#include "Scope.h"
#include "Names.h"
#include "CoreTypes.h"
#include "Symbols.h"
#include "Control.h"
#include "Literals.h"
#include "QtContextKeywords.h"
#include <string>
#include <cassert>

using namespace CPlusPlus;

CheckDeclaration::CheckDeclaration(Semantic *semantic)
    : SemanticCheck(semantic),
      _declaration(0),
      _scope(0),
      _checkAnonymousArguments(false)
{ }

CheckDeclaration::~CheckDeclaration()
{ }

void CheckDeclaration::check(DeclarationAST *declaration, Scope *scope)
{
    Scope *previousScope = switchScope(scope);
    DeclarationAST *previousDeclaration = switchDeclaration(declaration);
    accept(declaration);
    (void) switchDeclaration(previousDeclaration);
    (void) switchScope(previousScope);
}

void CheckDeclaration::check(CtorInitializerAST *ast, Scope *scope)
{
    Scope *previousScope = switchScope(scope);
    accept(ast);
    (void) switchScope(previousScope);
}

DeclarationAST *CheckDeclaration::switchDeclaration(DeclarationAST *declaration)
{
    DeclarationAST *previousDeclaration = _declaration;
    _declaration = declaration;
    return previousDeclaration;
}

Scope *CheckDeclaration::switchScope(Scope *scope)
{
    Scope *previousScope = _scope;
    _scope = scope;
    return previousScope;
}

void CheckDeclaration::setDeclSpecifiers(Symbol *symbol, const FullySpecifiedType &declSpecifiers)
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

void CheckDeclaration::checkFunctionArguments(Function *fun)
{
    if (! _checkAnonymousArguments)
        return;

    if (_scope->isClass() && fun->isPublic()) {
        for (unsigned argc = 0; argc < fun->argumentCount(); ++argc) {
            Argument *arg = fun->argumentAt(argc)->asArgument();
            assert(arg != 0);

            if (! arg->name()) {
                translationUnit()->warning(arg->sourceLocation(),
                                           "anonymous argument");
            }
        }
    }
}

bool CheckDeclaration::visit(SimpleDeclarationAST *ast)
{
    FullySpecifiedType declSpecifiers = semantic()->check(ast->decl_specifier_list, _scope);
    FullySpecifiedType qualTy = declSpecifiers.qualifiedType();

    if (ast->decl_specifier_list && ! ast->declarator_list) {
        ElaboratedTypeSpecifierAST *elab_type_spec = ast->decl_specifier_list->value->asElaboratedTypeSpecifier();

        if (! elab_type_spec && declSpecifiers.isFriend() && ast->decl_specifier_list->next && ! ast->decl_specifier_list->next->next) {
            // friend template class
            elab_type_spec = ast->decl_specifier_list->next->value->asElaboratedTypeSpecifier();
        }

        if (elab_type_spec) {
            unsigned sourceLocation = ast->decl_specifier_list->firstToken();

            const Name *name = semantic()->check(elab_type_spec->name, _scope);
            ForwardClassDeclaration *symbol =
                    control()->newForwardClassDeclaration(sourceLocation, name);

            setDeclSpecifiers(symbol, declSpecifiers);

            _scope->addMember(symbol);
            return false;
        }
    }

    const bool isQ_SLOT   = ast->qt_invokable_token && tokenKind(ast->qt_invokable_token) == T_Q_SLOT;
    const bool isQ_SIGNAL = ast->qt_invokable_token && tokenKind(ast->qt_invokable_token) == T_Q_SIGNAL;
    const bool isQ_INVOKABLE = ast->qt_invokable_token && tokenKind(ast->qt_invokable_token) == T_Q_INVOKABLE;

    List<Symbol *> **decl_it = &ast->symbols;
    for (DeclaratorListAST *it = ast->declarator_list; it; it = it->next) {
        const Name *name = 0;
        FullySpecifiedType declTy = semantic()->check(it->value, qualTy,
                                                      _scope, &name);

        unsigned location = semantic()->location(it->value);
        if (! location)
            location = ast->firstToken();

        Function *fun = 0;
        if (declTy && 0 != (fun = declTy->asFunctionType())) {
            fun->setSourceLocation(location, translationUnit());
            fun->setScope(_scope);
            fun->setName(name);
            fun->setMethodKey(semantic()->currentMethodKey());

            setDeclSpecifiers(fun, declSpecifiers);

            if (isQ_SIGNAL)
                fun->setMethodKey(Function::SignalMethod);
            else if (isQ_SLOT)
                fun->setMethodKey(Function::SlotMethod);
            else if (isQ_INVOKABLE)
                fun->setMethodKey(Function::InvokableMethod);
            fun->setVisibility(semantic()->currentVisibility());
        } else if (semantic()->currentMethodKey() != Function::NormalMethod) {
            translationUnit()->warning(ast->firstToken(),
                                       "expected a function declaration");
        }

        Declaration *symbol = control()->newDeclaration(location, name);

        symbol->setType(declTy);

        setDeclSpecifiers(symbol, declSpecifiers);

        symbol->setVisibility(semantic()->currentVisibility());

        if (it->value && it->value->initializer) {
            FullySpecifiedType initTy = semantic()->check(it->value->initializer, _scope);
        }

        *decl_it = new (translationUnit()->memoryPool()) List<Symbol *>();
        (*decl_it)->value = symbol;
        decl_it = &(*decl_it)->next;

        _scope->addMember(symbol);
    }
    return false;
}

bool CheckDeclaration::visit(EmptyDeclarationAST *)
{
    return false;
}

bool CheckDeclaration::visit(AccessDeclarationAST *ast)
{
    int accessSpecifier = tokenKind(ast->access_specifier_token);
    int visibility = semantic()->visibilityForAccessSpecifier(accessSpecifier);
    semantic()->switchVisibility(visibility);
    if (ast->slots_token)
        semantic()->switchMethodKey(Function::SlotMethod);
    else if (accessSpecifier == T_Q_SIGNALS)
        semantic()->switchMethodKey(Function::SignalMethod);
    else
        semantic()->switchMethodKey(Function::NormalMethod);
    return false;
}

bool CheckDeclaration::visit(AsmDefinitionAST *)
{
    return false;
}

bool CheckDeclaration::visit(ExceptionDeclarationAST *ast)
{
    FullySpecifiedType ty = semantic()->check(ast->type_specifier_list, _scope);
    FullySpecifiedType qualTy = ty.qualifiedType();

    const Name *name = 0;
    FullySpecifiedType declTy = semantic()->check(ast->declarator, qualTy,
                                                  _scope, &name);

    unsigned location = semantic()->location(ast->declarator);
    if (! location) {
        if (ast->declarator)
            location = ast->declarator->firstToken();
        else
            location = ast->firstToken();
    }

    Declaration *symbol = control()->newDeclaration(location, name);
    symbol->setType(declTy);
    _scope->addMember(symbol);

    return false;
}

bool CheckDeclaration::visit(FunctionDefinitionAST *ast)
{
    FullySpecifiedType ty = semantic()->check(ast->decl_specifier_list, _scope);
    FullySpecifiedType qualTy = ty.qualifiedType();
    const Name *name = 0;
    FullySpecifiedType funTy = semantic()->check(ast->declarator, qualTy,
                                                 _scope, &name);
    if (! funTy->isFunctionType()) {
        translationUnit()->error(ast->firstToken(),
                                 "expected a function prototype");
        return false;
    }

    unsigned funStartOffset = tokenAt(ast->firstToken()).offset;
    if (ast->declarator && ast->declarator->core_declarator) {
        funStartOffset = tokenAt(ast->declarator->core_declarator->lastToken() - 1).end();
    }

    Function *fun = funTy->asFunctionType();
    setDeclSpecifiers(fun, ty);

    fun->setStartOffset(funStartOffset);
    fun->setEndOffset(tokenAt(ast->lastToken() - 1).end());
    if (ast->declarator) {
        unsigned loc = semantic()->location(ast->declarator);
        if (! loc)
            loc = ast->declarator->firstToken();
        fun->setSourceLocation(loc, translationUnit());
    }
    fun->setName(name);
    fun->setVisibility(semantic()->currentVisibility());
    fun->setMethodKey(semantic()->currentMethodKey());

    const bool isQ_SLOT   = ast->qt_invokable_token && tokenKind(ast->qt_invokable_token) == T_Q_SLOT;
    const bool isQ_SIGNAL = ast->qt_invokable_token && tokenKind(ast->qt_invokable_token) == T_Q_SIGNAL;
    const bool isQ_INVOKABLE = ast->qt_invokable_token && tokenKind(ast->qt_invokable_token) == T_Q_INVOKABLE;

    if (isQ_SIGNAL)
        fun->setMethodKey(Function::SignalMethod);
    else if (isQ_SLOT)
        fun->setMethodKey(Function::SlotMethod);
    else if (isQ_INVOKABLE)
        fun->setMethodKey(Function::InvokableMethod);

    checkFunctionArguments(fun);

    ast->symbol = fun;
    _scope->addMember(fun);

    if (! semantic()->skipFunctionBodies())
        semantic()->checkFunctionDefinition(ast);

    return false;
}

bool CheckDeclaration::visit(MemInitializerAST *ast)
{
    (void) semantic()->check(ast->name, _scope);
    for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
        FullySpecifiedType ty = semantic()->check(it->value, _scope);
    }
    return false;
}

bool CheckDeclaration::visit(LinkageBodyAST *ast)
{
    for (DeclarationListAST *decl = ast->declaration_list; decl; decl = decl->next) {
       semantic()->check(decl->value, _scope);
    }
    return false;
}

bool CheckDeclaration::visit(LinkageSpecificationAST *ast)
{
    semantic()->check(ast->declaration, _scope);
    return false;
}

bool CheckDeclaration::visit(NamespaceAST *ast)
{
    const Name *namespaceName = 0;
    if (const Identifier *id = identifier(ast->identifier_token))
        namespaceName = control()->nameId(id);

    unsigned sourceLocation = ast->firstToken();
    if (ast->identifier_token)
        sourceLocation = ast->identifier_token;
    unsigned scopeStart = tokenAt(ast->firstToken()).offset;
    if (ast->linkage_body && ast->linkage_body->firstToken())
        scopeStart = tokenAt(ast->linkage_body->firstToken()).offset;

    Namespace *ns = control()->newNamespace(sourceLocation, namespaceName);
    ns->setStartOffset(scopeStart);
    ns->setEndOffset(tokenAt(ast->lastToken() - 1).end());
    ast->symbol = ns;
    _scope->addMember(ns);
    semantic()->check(ast->linkage_body, ns); // ### we'll do the merge later.

    return false;
}

bool CheckDeclaration::visit(NamespaceAliasDefinitionAST *ast)
{
    const Name *name = 0;

    if (const Identifier *id = identifier(ast->namespace_name_token))
        name = control()->nameId(id);

    unsigned sourceLocation = ast->firstToken();

    if (ast->namespace_name_token)
        sourceLocation = ast->namespace_name_token;

    const Name *namespaceName = semantic()->check(ast->name, _scope);

    NamespaceAlias *namespaceAlias = control()->newNamespaceAlias(sourceLocation, name);
    namespaceAlias->setNamespaceName(namespaceName);
    //ast->symbol = namespaceAlias;
    _scope->addMember(namespaceAlias);

    return false;
}

bool CheckDeclaration::visit(ParameterDeclarationAST *ast)
{
    unsigned sourceLocation = semantic()->location(ast->declarator);
    if (! sourceLocation) {
        if (ast->declarator)
            sourceLocation = ast->declarator->firstToken();
        else
            sourceLocation = ast->firstToken();
    }

    const Name *argName = 0;
    FullySpecifiedType ty = semantic()->check(ast->type_specifier_list, _scope);
    FullySpecifiedType argTy = semantic()->check(ast->declarator, ty.qualifiedType(),
                                                 _scope, &argName);
    FullySpecifiedType exprTy = semantic()->check(ast->expression, _scope);
    Argument *arg = control()->newArgument(sourceLocation, argName);
    ast->symbol = arg;
    if (ast->expression) {
        unsigned startOfExpression = ast->expression->firstToken();
        unsigned endOfExpression = ast->expression->lastToken();
        std::string buffer;
        for (unsigned index = startOfExpression; index != endOfExpression; ++index) {
            const Token &tk = tokenAt(index);
            if (tk.whitespace() || tk.newline())
                buffer += ' ';
            buffer += tk.spell();
        }
        const StringLiteral *initializer = control()->stringLiteral(buffer.c_str(), buffer.size());
        arg->setInitializer(initializer);
    }
    arg->setType(argTy);
    _scope->addMember(arg);
    return false;
}

bool CheckDeclaration::visit(TemplateDeclarationAST *ast)
{
    Template *templ = control()->newTemplate(ast->firstToken());
    ast->symbol = templ;

    for (DeclarationListAST *param = ast->template_parameter_list; param; param = param->next) {
       semantic()->check(param->value, templ);
    }

    semantic()->check(ast->declaration, templ);

    if (Symbol *decl = templ->declaration()) {
        // propagate the name
        if (decl->sourceLocation())
            templ->setSourceLocation(decl->sourceLocation(), translationUnit());
        templ->setName(decl->name());
    }

    _scope->addMember(templ);

    return false;
}

bool CheckDeclaration::visit(TypenameTypeParameterAST *ast)
{
    unsigned sourceLocation = ast->firstToken();
    if (ast->name)
        sourceLocation = ast->name->firstToken();

    const Name *name = semantic()->check(ast->name, _scope);
    TypenameArgument *arg = control()->newTypenameArgument(sourceLocation, name);
    FullySpecifiedType ty = semantic()->check(ast->type_id, _scope);
    arg->setType(ty);
    ast->symbol = arg;
    _scope->addMember(arg);
    return false;
}

bool CheckDeclaration::visit(TemplateTypeParameterAST *ast)
{
    unsigned sourceLocation = ast->firstToken();
    if (ast->name)
        sourceLocation = ast->name->firstToken();

    const Name *name = semantic()->check(ast->name, _scope);
    TypenameArgument *arg = control()->newTypenameArgument(sourceLocation, name);
    FullySpecifiedType ty = semantic()->check(ast->type_id, _scope);
    arg->setType(ty);
    ast->symbol = arg;
    _scope->addMember(arg);
    return false;
}

bool CheckDeclaration::visit(UsingAST *ast)
{
    const Name *name = semantic()->check(ast->name, _scope);

    unsigned sourceLocation = ast->firstToken();
    if (ast->name)
        sourceLocation = ast->name->firstToken();

    UsingDeclaration *u = control()->newUsingDeclaration(sourceLocation, name);
    ast->symbol = u;
    _scope->addMember(u);
    return false;
}

bool CheckDeclaration::visit(UsingDirectiveAST *ast)
{
    const Name *name = semantic()->check(ast->name, _scope);

    unsigned sourceLocation = ast->firstToken();
    if (ast->name)
        sourceLocation = ast->name->firstToken();

    UsingNamespaceDirective *u = control()->newUsingNamespaceDirective(sourceLocation, name);
    ast->symbol = u;
    _scope->addMember(u);

    if (! (_scope->isBlock() || _scope->isNamespace()))
        translationUnit()->error(ast->firstToken(),
                                 "using-directive not within namespace or block scope");

    return false;
}

bool CheckDeclaration::visit(ObjCProtocolForwardDeclarationAST *ast)
{
    const unsigned sourceLocation = ast->firstToken();

    List<ObjCForwardProtocolDeclaration *> **symbolIter = &ast->symbols;
    for (NameListAST *it = ast->identifier_list; it; it = it->next) {
        unsigned declarationLocation;
        if (it->value)
            declarationLocation = it->value->firstToken();
        else
            declarationLocation = sourceLocation;

        const Name *protocolName = semantic()->check(it->value, _scope);
        ObjCForwardProtocolDeclaration *fwdProtocol = control()->newObjCForwardProtocolDeclaration(sourceLocation, protocolName);

        _scope->addMember(fwdProtocol);

        *symbolIter = new (translationUnit()->memoryPool()) List<ObjCForwardProtocolDeclaration *>();
        (*symbolIter)->value = fwdProtocol;
        symbolIter = &(*symbolIter)->next;
    }

    return false;
}

unsigned CheckDeclaration::calculateScopeStart(ObjCProtocolDeclarationAST *ast) const
{
    if (ast->protocol_refs)
        if (unsigned pos = ast->protocol_refs->lastToken())
            return tokenAt(pos - 1).end();
    if (ast->name)
        if (unsigned pos = ast->name->lastToken())
            return tokenAt(pos - 1).end();

    return tokenAt(ast->firstToken()).offset;
}

bool CheckDeclaration::visit(ObjCProtocolDeclarationAST *ast)
{
    unsigned sourceLocation;
    if (ast->name)
        sourceLocation = ast->name->firstToken();
    else
        sourceLocation = ast->firstToken();

    const Name *protocolName = semantic()->check(ast->name, _scope);
    ObjCProtocol *protocol = control()->newObjCProtocol(sourceLocation, protocolName);
    protocol->setStartOffset(calculateScopeStart(ast));
    protocol->setEndOffset(tokenAt(ast->lastToken() - 1).end());

    if (ast->protocol_refs && ast->protocol_refs->identifier_list) {
        for (NameListAST *iter = ast->protocol_refs->identifier_list; iter; iter = iter->next) {
            NameAST* name = iter->value;
            const Name *protocolName = semantic()->check(name, _scope);
            ObjCBaseProtocol *baseProtocol = control()->newObjCBaseProtocol(name->firstToken(), protocolName);
            protocol->addProtocol(baseProtocol);
        }
    }

    int previousObjCVisibility = semantic()->switchObjCVisibility(Function::Public);
    for (DeclarationListAST *it = ast->member_declaration_list; it; it = it->next) {
        semantic()->check(it->value, protocol);
    }
    (void) semantic()->switchObjCVisibility(previousObjCVisibility);

    ast->symbol = protocol;
    _scope->addMember(protocol);

    return false;
}

bool CheckDeclaration::visit(ObjCClassForwardDeclarationAST *ast)
{
    const unsigned sourceLocation = ast->firstToken();

    List<ObjCForwardClassDeclaration *> **symbolIter = &ast->symbols;
    for (NameListAST *it = ast->identifier_list; it; it = it->next) {
        unsigned declarationLocation;
        if (it->value)
            declarationLocation = it->value->firstToken();
        else
            declarationLocation = sourceLocation;

        const Name *className = semantic()->check(it->value, _scope);
        ObjCForwardClassDeclaration *fwdClass = control()->newObjCForwardClassDeclaration(sourceLocation, className);

        _scope->addMember(fwdClass);

        *symbolIter = new (translationUnit()->memoryPool()) List<ObjCForwardClassDeclaration *>();
        (*symbolIter)->value = fwdClass;
        symbolIter = &(*symbolIter)->next;
    }

    return false;
}

unsigned CheckDeclaration::calculateScopeStart(ObjCClassDeclarationAST *ast) const
{
    if (ast->inst_vars_decl)
        if (unsigned pos = ast->inst_vars_decl->lbrace_token)
            return tokenAt(pos).end();

    if (ast->protocol_refs)
        if (unsigned pos = ast->protocol_refs->lastToken())
            return tokenAt(pos - 1).end();

    if (ast->superclass)
        if (unsigned pos = ast->superclass->lastToken())
            return tokenAt(pos - 1).end();

    if (ast->colon_token)
        return tokenAt(ast->colon_token).end();

    if (ast->rparen_token)
        return tokenAt(ast->rparen_token).end();

    if (ast->category_name)
        if (unsigned pos = ast->category_name->lastToken())
            return tokenAt(pos - 1).end();

    if (ast->lparen_token)
        return tokenAt(ast->lparen_token).end();

    if (ast->class_name)
        if (unsigned pos = ast->class_name->lastToken())
            return tokenAt(pos - 1).end();

    return tokenAt(ast->firstToken()).offset;
}

bool CheckDeclaration::visit(ObjCClassDeclarationAST *ast)
{
    unsigned sourceLocation;
    if (ast->class_name)
        sourceLocation = ast->class_name->firstToken();
    else
        sourceLocation = ast->firstToken();

    const Name *className = semantic()->check(ast->class_name, _scope);
    ObjCClass *klass = control()->newObjCClass(sourceLocation, className);
    klass->setStartOffset(calculateScopeStart(ast));
    klass->setEndOffset(tokenAt(ast->lastToken() - 1).offset);
    ast->symbol = klass;

    klass->setInterface(ast->interface_token != 0);

    if (ast->category_name) {
        const Name *categoryName = semantic()->check(ast->category_name, _scope);
        klass->setCategoryName(categoryName);
    }

    if (ast->superclass) {
        const Name *superClassName = semantic()->check(ast->superclass, _scope);
        ObjCBaseClass *superKlass = control()->newObjCBaseClass(ast->superclass->firstToken(), superClassName);
        klass->setBaseClass(superKlass);
    }

    if (ast->protocol_refs && ast->protocol_refs->identifier_list) {
        for (NameListAST *iter = ast->protocol_refs->identifier_list; iter; iter = iter->next) {
            NameAST* name = iter->value;
            const Name *protocolName = semantic()->check(name, _scope);
            ObjCBaseProtocol *baseProtocol = control()->newObjCBaseProtocol(name->firstToken(), protocolName);
            klass->addProtocol(baseProtocol);
        }
    }

    _scope->addMember(klass);

    int previousObjCVisibility = semantic()->switchObjCVisibility(Function::Protected);

    if (ast->inst_vars_decl) {
        for (DeclarationListAST *it = ast->inst_vars_decl->instance_variable_list; it; it = it->next) {
            semantic()->check(it->value, klass);
        }
    }

    (void) semantic()->switchObjCVisibility(Function::Public);

    for (DeclarationListAST *it = ast->member_declaration_list; it; it = it->next) {
        semantic()->check(it->value, klass);
    }

    (void) semantic()->switchObjCVisibility(previousObjCVisibility);

    return false;
}

bool CheckDeclaration::visit(ObjCMethodDeclarationAST *ast)
{
    ObjCMethodPrototypeAST *methodProto = ast->method_prototype;
    if (!methodProto)
        return false;
    ObjCSelectorAST *selector = methodProto->selector;
    if (!selector)
        return false;

    FullySpecifiedType ty = semantic()->check(methodProto, _scope);
    ObjCMethod *methodTy = ty.type()->asObjCMethodType();
    if (!methodTy)
        return false;

    Symbol *symbol;
    if (ast->function_body) {
        symbol = methodTy;
        methodTy->setStartOffset(tokenAt(ast->function_body->firstToken()).offset);
        methodTy->setEndOffset(tokenAt(ast->lastToken() - 1).end());
    } else {
        Declaration *decl = control()->newDeclaration(selector->firstToken(), methodTy->name());
        decl->setType(methodTy);
        symbol = decl;
        symbol->setStorage(methodTy->storage());
    }

    symbol->setVisibility(semantic()->currentObjCVisibility());
    if (ty.isDeprecated())
        symbol->setDeprecated(true);
    if (ty.isUnavailable())
        symbol->setUnavailable(true);

    _scope->addMember(symbol);

    if (ast->function_body && !semantic()->skipFunctionBodies()) {
        semantic()->check(ast->function_body, methodTy);
    }

    return false;
}

bool CheckDeclaration::visit(ObjCVisibilityDeclarationAST *ast)
{
    int accessSpecifier = tokenKind(ast->visibility_token);
    int visibility = semantic()->visibilityForObjCAccessSpecifier(accessSpecifier);
    semantic()->switchObjCVisibility(visibility);
    return false;
}

bool CheckDeclaration::checkPropertyAttribute(ObjCPropertyAttributeAST *attrAst,
                                              int &flags,
                                              int attr)
{
    if (flags & attr) {
        translationUnit()->warning(attrAst->attribute_identifier_token,
                                   "duplicate property attribute \"%s\"",
                                   spell(attrAst->attribute_identifier_token));
        return false;
    } else {
        flags |= attr;
        return true;
    }
}

bool CheckDeclaration::visit(ObjCPropertyDeclarationAST *ast)
{
    semantic()->check(ast->simple_declaration, _scope);
    SimpleDeclarationAST *simpleDecl = ast->simple_declaration->asSimpleDeclaration();

    if (!simpleDecl) {
        translationUnit()->warning(ast->simple_declaration->firstToken(),
                                   "invalid type for property declaration");
        return false;
    }

    int propAttrs = ObjCPropertyDeclaration::None;
    const Name *getterName = 0, *setterName = 0;

    for (ObjCPropertyAttributeListAST *iter= ast->property_attribute_list; iter; iter = iter->next) {
        ObjCPropertyAttributeAST *attrAst = iter->value;
        if (!attrAst)
            continue;

        const Identifier *attrId = identifier(attrAst->attribute_identifier_token);
        if (attrId == control()->objcGetterId()) {
            if (checkPropertyAttribute(attrAst, propAttrs, ObjCPropertyDeclaration::Getter)) {
                getterName = semantic()->check(attrAst->method_selector, _scope);
            }
        } else if (attrId == control()->objcSetterId()) {
            if (checkPropertyAttribute(attrAst, propAttrs, ObjCPropertyDeclaration::Setter)) {
                setterName = semantic()->check(attrAst->method_selector, _scope);
            }
        } else if (attrId == control()->objcReadwriteId()) {
            checkPropertyAttribute(attrAst, propAttrs, ObjCPropertyDeclaration::ReadWrite);
        } else if (attrId == control()->objcReadonlyId()) {
            checkPropertyAttribute(attrAst, propAttrs, ObjCPropertyDeclaration::ReadOnly);
        } else if (attrId == control()->objcAssignId()) {
            checkPropertyAttribute(attrAst, propAttrs, ObjCPropertyDeclaration::Assign);
        } else if (attrId == control()->objcRetainId()) {
            checkPropertyAttribute(attrAst, propAttrs, ObjCPropertyDeclaration::Retain);
        } else if (attrId == control()->objcCopyId()) {
            checkPropertyAttribute(attrAst, propAttrs, ObjCPropertyDeclaration::Copy);
        } else if (attrId == control()->objcNonatomicId()) {
            checkPropertyAttribute(attrAst, propAttrs, ObjCPropertyDeclaration::NonAtomic);
        }
    }

    if (propAttrs & ObjCPropertyDeclaration::ReadOnly &&
        propAttrs & ObjCPropertyDeclaration::ReadWrite)
        // Should this be an error instead of only a warning?
        translationUnit()->warning(ast->property_token,
                                   "property can have at most one attribute \"readonly\" or \"readwrite\" specified");
    int setterSemAttrs = propAttrs & ObjCPropertyDeclaration::SetterSemanticsMask;
    if (setterSemAttrs
            && setterSemAttrs != ObjCPropertyDeclaration::Assign
            && setterSemAttrs != ObjCPropertyDeclaration::Retain
            && setterSemAttrs != ObjCPropertyDeclaration::Copy) {
        // Should this be an error instead of only a warning?
        translationUnit()->warning(ast->property_token,
                                   "property can have at most one attribute \"assign\", \"retain\", or \"copy\" specified");
    }

    List<ObjCPropertyDeclaration *> **lastSymbols = &ast->symbols;
    for (List<Symbol *> *iter = simpleDecl->symbols; iter; iter = iter->next) {
        ObjCPropertyDeclaration *propDecl = control()->newObjCPropertyDeclaration(ast->firstToken(),
                                                                                  iter->value->name());
        propDecl->setType(iter->value->type());
        propDecl->setAttributes(propAttrs);
        propDecl->setGetterName(getterName);
        propDecl->setSetterName(setterName);
        _scope->addMember(propDecl);

        *lastSymbols = new (translationUnit()->memoryPool()) List<ObjCPropertyDeclaration *>();
        (*lastSymbols)->value = propDecl;
        lastSymbols = &(*lastSymbols)->next;
    }

    return false;
}

bool CheckDeclaration::visit(QtEnumDeclarationAST *ast)
{
    checkQEnumsQFlagsNames(ast->enumerator_list, "Q_ENUMS");
    return false;
}

bool CheckDeclaration::visit(QtFlagsDeclarationAST *ast)
{
    checkQEnumsQFlagsNames(ast->flag_enums_list, "Q_FLAGS");
    return false;
}

bool CheckDeclaration::visit(QtPropertyDeclarationAST *ast)
{
    if (ast->type_id)
        semantic()->check(ast->type_id, _scope);
    if (ast->property_name)
        semantic()->check(ast->property_name, _scope);
    for (QtPropertyDeclarationItemListAST *iter = ast->property_declaration_items;
         iter; iter = iter->next) {
        if (iter->value)
            semantic()->check(iter->value->expression, _scope);
    }
    return false;
}

static bool checkEnumName(const Name *name)
{
    if (! name)
        return false;

    else if (name->asNameId() != 0)
        return true;

    else if (const QualifiedNameId *q = name->asQualifiedNameId()) {
        if (! q->base())
            return false; // global qualified name

        if (checkEnumName(q->base()) && checkEnumName(q->name()))
            return true;
    }

    return false;
}

void CheckDeclaration::checkQEnumsQFlagsNames(NameListAST *nameListAst, const char *declName)
{
    for (NameListAST *iter = nameListAst; iter; iter = iter->next) {
        if (const Name *name = semantic()->check(iter->value, _scope)) {
            if (! checkEnumName(name))
                translationUnit()->error(iter->firstToken(), "invalid name in %s", declName);
        }
    }
}
