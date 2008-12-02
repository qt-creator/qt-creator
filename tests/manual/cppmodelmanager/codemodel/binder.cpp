/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** 
** Non-Open Source Usage  
** 
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.  
** 
** GNU General Public License Usage 
** 
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
***************************************************************************/
/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

/* This file is part of KDevelop
Copyright (C) 2002-2005 Roberto Raggi <roberto@kdevelop.org>
Copyright (C) 2005 Trolltech AS

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License version 2 as published by the Free Software Foundation.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public License
along with this library; see the file COPYING.LIB.  If not, write to
the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
Boston, MA 02110-1301, USA.
*/

#include "binder.h"
#include "lexer.h"
#include "control.h"
#include "symbol.h"
#include "codemodel_finder.h"
#include "class_compiler.h"
#include "compiler_utils.h"
#include "tokens.h"
#include "dumptree.h"

#include <iostream>

#include <qdebug.h>

Binder::Binder(CppCodeModel *__model, LocationManager &__location, Control *__control)
: _M_model(__model),
_M_location(__location),
_M_token_stream(&_M_location.token_stream),
_M_control(__control),
_M_current_function_type(CodeModel::Normal),
type_cc(this),
name_cc(this),
decl_cc(this)
{
    _M_current_file = 0;
    _M_current_namespace = 0;
    _M_current_class = 0;
    _M_current_function = 0;
    _M_current_enum = 0;
}

Binder::~Binder()
{
}

void Binder::run(AST *node, const QString &filename)
{
    _M_current_access = CodeModel::Public;
    if (_M_current_file = model()->fileItem(filename))
        visit(node);
}

ScopeModelItem *Binder::findScope(const QString &name) const
{
    return _M_known_scopes.value(name, 0);
}

ScopeModelItem *Binder::resolveScope(NameAST *id, ScopeModelItem *scope)
{
    Q_ASSERT(scope != 0);

    bool foundScope;
    CodeModelFinder finder(model(), this);
    QString symbolScopeName = finder.resolveScope(id, scope, &foundScope);
    if (!foundScope) {
        name_cc.run(id);
        std::cerr << "** WARNING scope not found for symbol:"
            << qPrintable(name_cc.qualifiedName()) << std::endl;
        return 0;
    }

    if (symbolScopeName.isEmpty())
        return scope;

    ScopeModelItem *symbolScope = findScope(symbolScopeName);

    qDebug() << "Resolving: " << symbolScopeName;
    qDebug() << " Current File: " << scope->file()->name();

    if (symbolScope)
        qDebug() << " Found in file: " << symbolScope->file()->name();

    if (!symbolScope || symbolScope->file() != scope->file()) {
        CppFileModelItem *file = model_cast<CppFileModelItem *>(scope->file());
        symbolScope = file->findExternalScope(symbolScopeName);
        qDebug() << " Create as external reference";
        if (!symbolScope) {
            symbolScope = new ScopeModelItem(_M_model);
            symbolScope->setName(symbolScopeName);
            file->addExternalScope(symbolScope);
        }
    }

    return symbolScope;
}

ScopeModelItem *Binder::currentScope()
{
    if (_M_current_class)
        return _M_current_class;
    else if (_M_current_namespace)
        return _M_current_namespace;

    return _M_current_file;
}

TemplateParameterList Binder::changeTemplateParameters(TemplateParameterList templateParameters)
{
    TemplateParameterList old = _M_current_template_parameters;
    _M_current_template_parameters = templateParameters;
    return old;
}

CodeModel::FunctionType Binder::changeCurrentFunctionType(CodeModel::FunctionType functionType)
{
    CodeModel::FunctionType old = _M_current_function_type;
    _M_current_function_type = functionType;
    return old;
}

CodeModel::AccessPolicy Binder::changeCurrentAccess(CodeModel::AccessPolicy accessPolicy)
{
    CodeModel::AccessPolicy old = _M_current_access;
    _M_current_access = accessPolicy;
    return old;
}

NamespaceModelItem *Binder::changeCurrentNamespace(NamespaceModelItem *item)
{
    NamespaceModelItem *old = _M_current_namespace;
    _M_current_namespace = item;
    return old;
}

CppClassModelItem *Binder::changeCurrentClass(CppClassModelItem *item)
{
    CppClassModelItem *old = _M_current_class;
    _M_current_class = item;
    return old;
}

CppFunctionDefinitionModelItem *Binder::changeCurrentFunction(CppFunctionDefinitionModelItem *item)
{
    CppFunctionDefinitionModelItem *old = _M_current_function;
    _M_current_function = item;
    return old;
}

int Binder::decode_token(std::size_t index) const
{
    return _M_token_stream->kind(index);
}

CodeModel::AccessPolicy Binder::decode_access_policy(std::size_t index) const
{
    switch (decode_token(index))
    {
    case Token_class:
        return CodeModel::Private;

    case Token_struct:
    case Token_union:
        return CodeModel::Public;

    default:
        return CodeModel::Public;
    }
}

CodeModel::ClassType Binder::decode_class_type(std::size_t index) const
{
    switch (decode_token(index))
    {
    case Token_class:
        return CodeModel::Class;
    case Token_struct:
        return CodeModel::Struct;
    case Token_union:
        return CodeModel::Union;
    default:
        std::cerr << "** WARNING unrecognized class type" << std::endl;
    }
    return CodeModel::Class;
}

const NameSymbol *Binder::decode_symbol(std::size_t index) const
{
    return _M_token_stream->symbol(index);
}

void Binder::visitAccessSpecifier(AccessSpecifierAST *node)
{
    const ListNode<std::size_t> *it =  node->specs;
    if (it == 0)
        return;

    it = it->toFront();
    const ListNode<std::size_t> *end = it;

    do
    {
        switch (decode_token(it->element))
        {
        default:
            break;

        case Token_public:
            changeCurrentAccess(CodeModel::Public);
            changeCurrentFunctionType(CodeModel::Normal);
            break;
        case Token_protected:
            changeCurrentAccess(CodeModel::Protected);
            changeCurrentFunctionType(CodeModel::Normal);
            break;
        case Token_private:
            changeCurrentAccess(CodeModel::Private);
            changeCurrentFunctionType(CodeModel::Normal);
            break;
        case Token_signals:
            changeCurrentAccess(CodeModel::Protected);
            changeCurrentFunctionType(CodeModel::Signal);
            break;
        case Token_slots:
            changeCurrentFunctionType(CodeModel::Slot);
            break;
        }
        it = it->next;
    }
    while (it != end);
}

void Binder::visitSimpleDeclaration(SimpleDeclarationAST *node)
{
    visit(node->type_specifier);

    if (const ListNode<InitDeclaratorAST*> *it = node->init_declarators)
    {
        it = it->toFront();
        const ListNode<InitDeclaratorAST*> *end = it;
        do
        {
            InitDeclaratorAST *init_declarator = it->element;
            declare_symbol(node, init_declarator);
            it = it->next;
        }
        while (it != end);
    }
}

void Binder::declare_symbol(SimpleDeclarationAST *node, InitDeclaratorAST *init_declarator)
{
    DeclaratorAST *declarator = init_declarator->declarator;

    while (declarator && declarator->sub_declarator)
        declarator = declarator->sub_declarator;

    NameAST *id = declarator->id;
    if (! declarator->id)
    {
        std::cerr << "** WARNING expected a declarator id" << std::endl;
        return;
    }

    decl_cc.run(declarator);

    if (decl_cc.isFunction())
    {
        name_cc.run(id->unqualified_name);

        FunctionModelItem *fun = new FunctionModelItem(model());
        updateFileAndItemPosition (fun, node);

        ScopeModelItem *symbolScope = resolveScope(id, currentScope());
        if (!symbolScope) {
            delete fun;
            return;
        }

        fun->setAccessPolicy(_M_current_access);
        fun->setFunctionType(_M_current_function_type);
        fun->setName(name_cc.qualifiedName());
        fun->setAbstract(init_declarator->initializer != 0);
        fun->setConstant(declarator->fun_cv != 0);
        fun->setTemplateParameters(copyTemplateParameters(_M_current_template_parameters));
        applyStorageSpecifiers(node->storage_specifiers, fun);
        applyFunctionSpecifiers(node->function_specifiers, fun);

        // build the type
        TypeInfo *typeInfo = CompilerUtils::typeDescription(node->type_specifier,
            declarator,
            this);

        fun->setType(typeInfo);


        fun->setVariadics (decl_cc.isVariadics ());

        // ... and the signature
        foreach (DeclaratorCompiler::Parameter p, decl_cc.parameters())
        {
            CppArgumentModelItem *arg = new CppArgumentModelItem(model());
            arg->setType(p.type);
            arg->setName(p.name);
            arg->setDefaultValue(p.defaultValue);
            if (p.defaultValue)
                arg->setDefaultValueExpression(p.defaultValueExpression);
            fun->addArgument(arg);
        }

        fun->setScope(symbolScope->qualifiedName());
        symbolScope->addFunction(fun);
    }
    else
    {
        CppVariableModelItem *var = new CppVariableModelItem(model());
        updateFileAndItemPosition (var, node);

        ScopeModelItem *symbolScope = resolveScope(id, currentScope());
        if (!symbolScope) {
            delete var;
            return;
        }

        var->setTemplateParameters(copyTemplateParameters(_M_current_template_parameters));
        var->setAccessPolicy(_M_current_access);
        name_cc.run(id->unqualified_name);
        var->setName(name_cc.qualifiedName());
        TypeInfo *typeInfo = CompilerUtils::typeDescription(node->type_specifier,
            declarator,
            this);
        if (declarator != init_declarator->declarator
            && init_declarator->declarator->parameter_declaration_clause != 0)
        {
            typeInfo->setFunctionPointer (true);
            decl_cc.run (init_declarator->declarator);
            foreach (DeclaratorCompiler::Parameter p, decl_cc.parameters())
                typeInfo->addArgument(p.type);
        }

        var->setType(typeInfo);
        applyStorageSpecifiers(node->storage_specifiers, var);

        var->setScope(symbolScope->qualifiedName());
        symbolScope->addVariable(var);
    }
}

void Binder::visitFunctionDefinition(FunctionDefinitionAST *node)
{
    Q_ASSERT(node->init_declarator != 0);

    InitDeclaratorAST *init_declarator = node->init_declarator;
    DeclaratorAST *declarator = init_declarator->declarator;

    decl_cc.run(declarator);

    Q_ASSERT(! decl_cc.id().isEmpty());

    CppFunctionDefinitionModelItem *
        old = changeCurrentFunction(new CppFunctionDefinitionModelItem(_M_model));
    updateFileAndItemPosition (_M_current_function, node);

    ScopeModelItem *functionScope = resolveScope(declarator->id, currentScope());
    if (! functionScope) {
        delete _M_current_function;
        changeCurrentFunction(old);
        return;
    }

    _M_current_function->setScope(functionScope->qualifiedName());

    Q_ASSERT(declarator->id->unqualified_name != 0);
    name_cc.run(declarator->id->unqualified_name);
    QString unqualified_name = name_cc.qualifiedName();

    _M_current_function->setName(unqualified_name);
    TypeInfo *tmp_type = CompilerUtils::typeDescription(node->type_specifier,
        declarator, this);

    _M_current_function->setType(tmp_type);
    _M_current_function->setAccessPolicy(_M_current_access);
    _M_current_function->setFunctionType(_M_current_function_type);
    _M_current_function->setConstant(declarator->fun_cv != 0);
    _M_current_function->setTemplateParameters(copyTemplateParameters(_M_current_template_parameters));

    applyStorageSpecifiers(node->storage_specifiers,
        _M_current_function);
    applyFunctionSpecifiers(node->function_specifiers,
        _M_current_function);

    _M_current_function->setVariadics (decl_cc.isVariadics ());

    foreach (DeclaratorCompiler::Parameter p, decl_cc.parameters())
    {
        CppArgumentModelItem *arg = new CppArgumentModelItem(model());
        arg->setType(p.type);
        arg->setName(p.name);
        arg->setDefaultValue(p.defaultValue);
        _M_current_function->addArgument(arg);
    }

    functionScope->addFunctionDefinition(_M_current_function);
    changeCurrentFunction(old);
}

void Binder::visitTemplateDeclaration(TemplateDeclarationAST *node)
{
    const ListNode<TemplateParameterAST*> *it = node->template_parameters;
    if (it == 0)
        return;

    TemplateParameterList savedTemplateParameters = changeTemplateParameters(TemplateParameterList());

    it = it->toFront();
    const ListNode<TemplateParameterAST*> *end = it;

    do
    {
        TemplateParameterAST *parameter = it->element;
        TypeParameterAST *type_parameter = parameter->type_parameter;
        if (! type_parameter)
        {
            std::cerr << "** WARNING template declaration not supported ``";
            Token const &tk = _M_token_stream->token ((int) node->start_token);
            Token const &end_tk = _M_token_stream->token ((int) node->declaration->start_token);

            std::cerr << std::string (&tk.text[tk.position], (end_tk.position) - tk.position) << "''"
                << std::endl << std::endl;

            qDeleteAll(_M_current_template_parameters);
            changeTemplateParameters(savedTemplateParameters);
            return;
        }
        assert(type_parameter != 0);

        int tk = decode_token(type_parameter->type);
        if (tk != Token_typename && tk != Token_class)
        {
            std::cerr << "** WARNING template declaration not supported ``";
            Token const &tk = _M_token_stream->token ((int) node->start_token);
            Token const &end_tk = _M_token_stream->token ((int) node->declaration->start_token);

            std::cerr << std::string (&tk.text[tk.position], (end_tk.position) - tk.position) << "''"
                << std::endl << std::endl;

            qDeleteAll(_M_current_template_parameters);
            changeTemplateParameters(savedTemplateParameters);
            return;
        }
        assert(tk == Token_typename || tk == Token_class);

        name_cc.run(type_parameter->name);

        CppTemplateParameterModelItem *p = new CppTemplateParameterModelItem(model());
        p->setName(name_cc.qualifiedName());
        _M_current_template_parameters.append(p);
        it = it->next;
    }
    while (it != end);

    visit(node->declaration);

    qDeleteAll(_M_current_template_parameters);
    changeTemplateParameters(savedTemplateParameters);
}

void Binder::visitTypedef(TypedefAST *node)
{
    const ListNode<InitDeclaratorAST*> *it = node->init_declarators;
    if (it == 0)
        return;

    it = it->toFront();
    const ListNode<InitDeclaratorAST*> *end = it;

    do
    {
        InitDeclaratorAST *init_declarator = it->element;
        it = it->next;

        Q_ASSERT(init_declarator->declarator != 0);

        // the name
        decl_cc.run (init_declarator->declarator);
        QString alias_name = decl_cc.id ();

        if (alias_name.isEmpty ())
        {
            std::cerr << "** WARNING anonymous typedef not supported! ``";
            Token const &tk = _M_token_stream->token ((int) node->start_token);
            Token const &end_tk = _M_token_stream->token ((int) node->end_token);

            std::cerr << std::string (&tk.text[tk.position], end_tk.position - tk.position) << "''"
                << std::endl << std::endl;
            continue;
        }

        // build the type
        TypeInfo *typeInfo = CompilerUtils::typeDescription (node->type_specifier,
            init_declarator->declarator,
            this);
        DeclaratorAST *decl = init_declarator->declarator;
        while (decl && decl->sub_declarator)
            decl = decl->sub_declarator;

        if (decl != init_declarator->declarator
            && init_declarator->declarator->parameter_declaration_clause != 0)
        {
            typeInfo->setFunctionPointer (true);
            decl_cc.run (init_declarator->declarator);
            foreach (DeclaratorCompiler::Parameter p, decl_cc.parameters())
                typeInfo->addArgument(p.type);
        }

        DeclaratorAST *declarator = init_declarator->declarator;

        CppTypeAliasModelItem *typeAlias = new CppTypeAliasModelItem(model());
        updateFileAndItemPosition (typeAlias, node);

        ScopeModelItem *typedefScope = resolveScope(declarator->id, currentScope());
        if (!typedefScope) {
            delete typeAlias;
            return;
        }

        typeAlias->setName (alias_name);
        typeAlias->setType (typeInfo);
        typeAlias->setScope (typedefScope->qualifiedName());
        typedefScope->addTypeAlias (typeAlias);
    }
    while (it != end);
}

void Binder::visitNamespace(NamespaceAST *node)
{
    bool anonymous = (node->namespace_name == 0);

    ScopeModelItem *scope = 0;
    NamespaceModelItem *old = 0;
    if (! anonymous)
    {
        // update the file if needed
        updateFileAndItemPosition (0, node);
        scope = currentScope();

        QString name = decode_symbol(node->namespace_name)->as_string();

        QString qualified_name = scope->qualifiedName();
        if (!qualified_name.isEmpty())
            qualified_name += QLatin1String("::");
        qualified_name += name;
        NamespaceModelItem *ns = model_cast<NamespaceModelItem *>(findScope(qualified_name));
        if (ns && ns->file() != scope->file()) {
            qDebug() << ns->file()->name() << " :: " << scope->file()->name();
            ns = 0; // we need a separate namespaces for different files
        }

        if (!ns)
        {
            ns = new NamespaceModelItem(_M_model);
            updateFileAndItemPosition (ns, node);
            
            _M_known_scopes.insert(qualified_name, ns);
            ns->setName(name);
            ns->setScope(scope->qualifiedName());
        }
        old = changeCurrentNamespace(ns);
    }

    DefaultVisitor::visitNamespace(node);

    if (! anonymous)
    {
        Q_ASSERT(scope->kind() == CodeModelItem::Kind_Namespace
            || scope->kind() == CodeModelItem::Kind_File);

        if (NamespaceModelItem *ns = model_cast<NamespaceModelItem *>(scope))
            ns->addNamespace(_M_current_namespace);

        changeCurrentNamespace(old);
    }
}

void Binder::visitClassSpecifier(ClassSpecifierAST *node)
{
    ClassCompiler class_cc(this);
    class_cc.run(node);

    if (class_cc.name().isEmpty())
    {
        // anonymous not supported
        return;
    }

    Q_ASSERT(node->name != 0 && node->name->unqualified_name != 0);


    CppClassModelItem *class_item = new CppClassModelItem(_M_model);
    updateFileAndItemPosition (class_item, node);
    ScopeModelItem *scope = currentScope();
    CppClassModelItem *old = changeCurrentClass(class_item);

    _M_current_class->setName(class_cc.name());
    _M_current_class->setBaseClasses(class_cc.baseClasses());
    _M_current_class->setClassType(decode_class_type(node->class_key));
    _M_current_class->setTemplateParameters(copyTemplateParameters(_M_current_template_parameters));

    QString name = _M_current_class->name();
    if (!_M_current_template_parameters.isEmpty())
    {
        name += "<";
        for (int i = 0; i<_M_current_template_parameters.size(); ++i)
        {
            if (i != 0)
                name += ",";

            name += _M_current_template_parameters.at(i)->name();
        }

        name += ">";
        _M_current_class->setName(name);
    }

    CodeModel::AccessPolicy oldAccessPolicy = changeCurrentAccess(decode_access_policy(node->class_key));
    CodeModel::FunctionType oldFunctionType = changeCurrentFunctionType(CodeModel::Normal);

    QString qualifiedname = scope->qualifiedName();
    _M_current_class->setScope(qualifiedname);
    if (!qualifiedname.isEmpty())
        qualifiedname += QLatin1String("::");
    qualifiedname += name;
    _M_known_scopes.insert(qualifiedname, _M_current_class);

    scope->addClass(_M_current_class);

    name_cc.run(node->name->unqualified_name);

    visitNodes(this, node->member_specs);

    changeCurrentClass(old);
    changeCurrentAccess(oldAccessPolicy);
    changeCurrentFunctionType(oldFunctionType);
}

void Binder::visitLinkageSpecification(LinkageSpecificationAST *node)
{
    DefaultVisitor::visitLinkageSpecification(node);
}

void Binder::visitUsing(UsingAST *node)
{
    DefaultVisitor::visitUsing(node);
}

void Binder::visitEnumSpecifier(EnumSpecifierAST *node)
{
    CodeModelFinder finder(model(), this);

    name_cc.run(node->name);
    QString name = name_cc.qualifiedName();

    if (name.isEmpty())
    {
        // anonymous enum
        static int N = 0;
        name = QLatin1String("$$enum_");
        name += QString::number(++N);
    }

    _M_current_enum = new CppEnumModelItem(model());

    updateFileAndItemPosition (_M_current_enum, node);
    ScopeModelItem *enumScope = resolveScope(node->name, currentScope());
    if (!enumScope) {
        delete _M_current_enum;
        _M_current_enum = 0;
        return;
    }


    _M_current_enum->setAccessPolicy(_M_current_access);
    _M_current_enum->setName(name);
    _M_current_enum->setScope(enumScope->qualifiedName());

    enumScope->addEnum(_M_current_enum);

    DefaultVisitor::visitEnumSpecifier(node);

    _M_current_enum = 0;
}

void Binder::visitEnumerator(EnumeratorAST *node)
{
    Q_ASSERT(_M_current_enum != 0);
    CppEnumeratorModelItem *e = new CppEnumeratorModelItem(model());
    updateFileAndItemPosition (e, node);
    e->setName(decode_symbol(node->id)->as_string());

    if (ExpressionAST *expr = node->expression)
    {
        const Token &start_token = _M_token_stream->token((int) expr->start_token);
        const Token &end_token = _M_token_stream->token((int) expr->end_token);

        e->setValue(QString::fromUtf8(&start_token.text[start_token.position],
            (int) (end_token.position - start_token.position)).trimmed());
    }

    _M_current_enum->addEnumerator(e);
}

void Binder::visitUsingDirective(UsingDirectiveAST *node)
{
    DefaultVisitor::visitUsingDirective(node);
}

void Binder::applyStorageSpecifiers(const ListNode<std::size_t> *it, MemberModelItem *item)
{
    if (it == 0)
        return;

    it = it->toFront();
    const ListNode<std::size_t> *end = it;

    do
    {
        switch (decode_token(it->element))
        {
        default:
            break;

        case Token_friend:
            item->setFriend(true);
            break;
        case Token_auto:
            item->setAuto(true);
            break;
        case Token_register:
            item->setRegister(true);
            break;
        case Token_static:
            item->setStatic(true);
            break;
        case Token_extern:
            item->setExtern(true);
            break;
        case Token_mutable:
            item->setMutable(true);
            break;
        }
        it = it->next;
    }
    while (it != end);
}

void Binder::applyFunctionSpecifiers(const ListNode<std::size_t> *it, FunctionModelItem *item)
{
    if (it == 0)
        return;

    it = it->toFront();
    const ListNode<std::size_t> *end = it;

    do
    {
        switch (decode_token(it->element))
        {
        default:
            break;

        case Token_inline:
            item->setInline(true);
            break;

        case Token_virtual:
            item->setVirtual(true);
            break;

        case Token_explicit:
            item->setExplicit(true);
            break;
        }
        it = it->next;
    }
    while (it != end);
}

void Binder::updateFileAndItemPosition(CodeModelItem *item, AST *node)
{
    QString filename;
    int sline, scolumn;
    int eline, ecolumn;

    assert (node != 0);
    _M_location.positionAt (_M_token_stream->position(node->start_token), &sline, &scolumn, &filename);
    _M_location.positionAt (_M_token_stream->position(node->end_token), &eline, &ecolumn, &QString());

    if (!filename.isEmpty() && (!_M_current_file || _M_current_file->name() != filename))
        _M_current_file = model()->fileItem(filename);

    if (item) {
        item->setFile(_M_current_file);
        item->setStartPosition(sline, scolumn);
        item->setEndPosition(eline, ecolumn);
    }
}

TemplateParameterList Binder::copyTemplateParameters(const TemplateParameterList &in) const
{
    TemplateParameterList result;
    foreach(TemplateParameterModelItem *item, in) {
        CppTemplateParameterModelItem *newitem = 
            new CppTemplateParameterModelItem(*(model_cast<CppTemplateParameterModelItem *>(item)));
        if (item->type()) {
            TypeInfo *type = new TypeInfo();
            *type = *(item->type());
            newitem->setType(type);
        }
        result.append(newitem);
    }
    return result;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
