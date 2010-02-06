/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "CheckUndefinedSymbols.h"
#include "Overview.h"

#include <Names.h>
#include <Literals.h>
#include <Symbols.h>
#include <TranslationUnit.h>
#include <Scope.h>
#include <AST.h>

using namespace CPlusPlus;


CheckUndefinedSymbols::CheckUndefinedSymbols(Document::Ptr doc)
    : ASTVisitor(doc->translationUnit()), _doc(doc)
{ }

CheckUndefinedSymbols::~CheckUndefinedSymbols()
{ }

void CheckUndefinedSymbols::setGlobalNamespaceBinding(NamespaceBindingPtr globalNamespaceBinding)
{
    _globalNamespaceBinding = globalNamespaceBinding;
    _types.clear();
    _protocols.clear();

    if (_globalNamespaceBinding) {
        QSet<NamespaceBinding *> processed;
        buildTypeMap(_globalNamespaceBinding.data(), &processed);
    }
}

void CheckUndefinedSymbols::operator()(AST *ast)
{ accept(ast); }

QByteArray CheckUndefinedSymbols::templateParameterName(NameAST *ast) const
{
    if (ast && ast->name) {
        if (const Identifier *id = ast->name->identifier())
            return QByteArray::fromRawData(id->chars(), id->size());
    }

    return QByteArray();
}

QByteArray CheckUndefinedSymbols::templateParameterName(DeclarationAST *ast) const
{
    if (ast) {
        if (TypenameTypeParameterAST *d = ast->asTypenameTypeParameter())
            return templateParameterName(d->name);
        else if (TemplateTypeParameterAST *d = ast->asTemplateTypeParameter())
            return templateParameterName(d->name);
        else if (ParameterDeclarationAST *d = ast->asParameterDeclaration()) {
            if (d->symbol) {
                if (const Identifier *id = d->symbol->identifier())
                    return QByteArray::fromRawData(id->chars(), id->size());
            }
        }
    }
    return QByteArray();
}

bool CheckUndefinedSymbols::isType(const QByteArray &name) const
{
    for (int i = _compoundStatementStack.size() - 1; i != -1; --i) {
        Scope *members = _compoundStatementStack.at(i)->symbol->members();

        for (unsigned m = 0; m < members->symbolCount(); ++m) {
            Symbol *member = members->symbolAt(m);

            if (member->isTypedef() && member->isDeclaration()) {
                if (const Identifier *id = member->identifier()) {
                    if (name == id->chars())
                        return true;
                }
            }
        }
    }

    for (int i = _templateDeclarationStack.size() - 1; i != - 1; --i) {
        TemplateDeclarationAST *templateDeclaration = _templateDeclarationStack.at(i);

        for (DeclarationListAST *it = templateDeclaration->template_parameter_list; it; it = it->next) {
            DeclarationAST *templateParameter = it->value;

            if (templateParameterName(templateParameter) == name)
                return true;
        }
    }

    return _types.contains(name);
}

bool CheckUndefinedSymbols::isType(const Identifier *id) const
{
    if (! id)
        return false;

    return isType(QByteArray::fromRawData(id->chars(), id->size()));
}

void CheckUndefinedSymbols::addType(const Name *name)
{
    if (! name)
        return;

    if (const Identifier *id = name->identifier())
        _types.insert(QByteArray(id->chars(), id->size()));
}

void CheckUndefinedSymbols::addProtocol(const Name *name)
{
    if (!name)
        return;

    if (const Identifier *id = name->identifier())
        _protocols.insert(QByteArray(id->chars(), id->size()));
}

bool CheckUndefinedSymbols::isProtocol(const QByteArray &name) const
{
    return _protocols.contains(name);
}

void CheckUndefinedSymbols::buildTypeMap(Class *klass)
{
    addType(klass->name());

    for (unsigned i = 0; i < klass->memberCount(); ++i) {
        buildMemberTypeMap(klass->memberAt(i));
    }
}

void CheckUndefinedSymbols::buildMemberTypeMap(Symbol *member)
{
    if (member == 0)
        return;

    if (Class *klass = member->asClass()) {
        buildTypeMap(klass);
    } else if (Enum *e = member->asEnum()) {
        addType(e->name());
    } else if (ForwardClassDeclaration *fwd = member->asForwardClassDeclaration()) {
        addType(fwd->name());
    } else if (Declaration *decl = member->asDeclaration()) {
        if (decl->isTypedef())
            addType(decl->name());
    }
}

void CheckUndefinedSymbols::buildTypeMap(NamespaceBinding *binding, QSet<NamespaceBinding *> *processed)
{
    if (! processed->contains(binding)) {
        processed->insert(binding);

        if (const Identifier *id = binding->identifier()) {
            _namespaceNames.insert(QByteArray(id->chars(), id->size()));
        }

        foreach (Namespace *ns, binding->symbols) {
            for (unsigned i = 0; i < ns->memberCount(); ++i) {
                Symbol *member = ns->memberAt(i);

                if (Class *klass = member->asClass()) {
                    buildTypeMap(klass);
                } else if (Enum *e = member->asEnum()) {
                    addType(e->name());
                } else if (ForwardClassDeclaration *fwd = member->asForwardClassDeclaration()) {
                    addType(fwd->name());
                } else if (Declaration *decl = member->asDeclaration()) {
                    if (decl->isTypedef())
                        addType(decl->name());
                } else if (ObjCForwardClassDeclaration *fKlass = member->asObjCForwardClassDeclaration()) {
                    addType(fKlass->name());
                } else if (ObjCClass *klass = member->asObjCClass()) {
                    addType(klass->name());

                    for (unsigned i = 0; i < klass->memberCount(); ++i)
                        buildMemberTypeMap(klass->memberAt(i));
                } else if (ObjCForwardProtocolDeclaration *fProto = member->asObjCForwardProtocolDeclaration()) {
                    addProtocol(fProto->name());
                } else if (ObjCProtocol *proto = member->asObjCProtocol()) {
                    addProtocol(proto->name());

                    for (unsigned i = 0; i < proto->memberCount(); ++i)
                        buildMemberTypeMap(proto->memberAt(i));
                }
            }
        }

        foreach (NamespaceBinding *childBinding, binding->children) {
            buildTypeMap(childBinding, processed);
        }
    }
}

FunctionDeclaratorAST *CheckUndefinedSymbols::currentFunctionDeclarator() const
{
    if (_functionDeclaratorStack.isEmpty())
        return 0;

    return _functionDeclaratorStack.last();
}

CompoundStatementAST *CheckUndefinedSymbols::compoundStatement() const
{
    if (_compoundStatementStack.isEmpty())
        return 0;

    return _compoundStatementStack.last();
}

bool CheckUndefinedSymbols::visit(FunctionDeclaratorAST *ast)
{
    _functionDeclaratorStack.append(ast);
    return true;
}

void CheckUndefinedSymbols::endVisit(FunctionDeclaratorAST *)
{
    _functionDeclaratorStack.removeLast();
}

bool CheckUndefinedSymbols::visit(TypeofSpecifierAST *)
{
    return false;
}

bool CheckUndefinedSymbols::visit(NamedTypeSpecifierAST *ast)
{
    if (ast->name) {
        if (! ast->name->name) {
            unsigned line, col;
            getTokenStartPosition(ast->firstToken(), &line, &col);
            // qWarning() << _doc->fileName() << line << col;
        } else if (const Identifier *id = ast->name->name->identifier()) {
            if (! isType(id)) {
                if (FunctionDeclaratorAST *functionDeclarator = currentFunctionDeclarator()) {
                    if (functionDeclarator->as_cpp_initializer)
                        return true;
                }

                Overview oo;
                translationUnit()->warning(ast->firstToken(), "`%s' is not a type name",
                                           qPrintable(oo(ast->name->name)));
            }
        }
    }
    return true;
}

bool CheckUndefinedSymbols::visit(TemplateDeclarationAST *ast)
{
    _templateDeclarationStack.append(ast);
    return true;
}

void CheckUndefinedSymbols::endVisit(TemplateDeclarationAST *)
{
    _templateDeclarationStack.removeLast();
}

bool CheckUndefinedSymbols::visit(ClassSpecifierAST *ast)
{
    bool hasQ_OBJECT_CHECK = false;

    if (ast->symbol) {
        Class *klass = ast->symbol->asClass();

        for (unsigned i = 0; i < klass->memberCount(); ++i) {
            Symbol *symbol = klass->memberAt(i);

            if (symbol->name() && symbol->name()->isNameId()) {
                const NameId *nameId = symbol->name()->asNameId();

                if (! qstrcmp(nameId->identifier()->chars(), "qt_check_for_QOBJECT_macro")) {
                    hasQ_OBJECT_CHECK = true;
                    break;
                }
            }
        }
    }

    _qobjectStack.append(hasQ_OBJECT_CHECK);

    return true;
}

void CheckUndefinedSymbols::endVisit(ClassSpecifierAST *)
{ _qobjectStack.removeLast(); }

bool CheckUndefinedSymbols::qobjectCheck() const
{
    if (_qobjectStack.isEmpty())
        return false;

    return _qobjectStack.last();
}

bool CheckUndefinedSymbols::visit(FunctionDefinitionAST *ast)
{
    if (ast->symbol) {
        Function *fun = ast->symbol->asFunction();
        if ((fun->isSignal() || fun->isSlot()) && ! qobjectCheck()) {
            translationUnit()->warning(ast->firstToken(),
                                       "you forgot the Q_OBJECT macro");
        }
    }
    return true;
}

void CheckUndefinedSymbols::endVisit(FunctionDefinitionAST *)
{ }

bool CheckUndefinedSymbols::visit(CompoundStatementAST *ast)
{
    _compoundStatementStack.append(ast);
    return true;
}

void CheckUndefinedSymbols::endVisit(CompoundStatementAST *)
{
    _compoundStatementStack.removeLast();
}

bool CheckUndefinedSymbols::visit(SimpleDeclarationAST *ast)
{
    const bool check = qobjectCheck();
    for (List<Declaration *> *it = ast->symbols; it; it = it->next) {
        Declaration *decl = it->value;

        if (Function *fun = decl->type()->asFunctionType()) {
            if ((fun->isSignal() || fun->isSlot()) && ! check) {
                translationUnit()->warning(ast->firstToken(),
                                           "you forgot the Q_OBJECT macro");
            }
        }
    }
    return true;
}

bool CheckUndefinedSymbols::visit(BaseSpecifierAST *base)
{
    if (NameAST *nameAST = base->name) {
        bool resolvedBaseClassName = false;

        if (const Name *name = nameAST->name) {
            const Identifier *id = name->identifier();
            const QByteArray spell = QByteArray::fromRawData(id->chars(), id->size());
            if (isType(spell))
                resolvedBaseClassName = true;
        }

        if (! resolvedBaseClassName)
            translationUnit()->warning(nameAST->firstToken(), "expected class-name");
    }

    return true;
}

bool CheckUndefinedSymbols::visit(UsingDirectiveAST *ast)
{
    if (ast->symbol && ast->symbol->name() && _globalNamespaceBinding) {
        const Location loc = Location(ast->symbol);

        NamespaceBinding *binding = _globalNamespaceBinding.data();

        if (Scope *enclosingNamespaceScope = ast->symbol->enclosingNamespaceScope())
            binding = NamespaceBinding::find(enclosingNamespaceScope->owner()->asNamespace(), binding);

        if (! binding || ! binding->resolveNamespace(loc, ast->symbol->name())) {
            translationUnit()->warning(ast->name->firstToken(),
                                       "expected a namespace");
        }
    }

    return true;
}

bool CheckUndefinedSymbols::visit(QualifiedNameAST *ast)
{
    if (ast->name) {
        const QualifiedNameId *q = ast->name->asQualifiedNameId();
        for (unsigned i = 0; i < q->nameCount() - 1; ++i) {
            const Name *name = q->nameAt(i);
            if (const Identifier *id = name->identifier()) {
                const QByteArray spell = QByteArray::fromRawData(id->chars(), id->size());
                if (! (_namespaceNames.contains(spell) || isType(id))) {
                    translationUnit()->warning(ast->firstToken(),
                                               "`%s' is not a namespace or class name",
                                               spell.constData());
                }
            }
        }
    }

    return true;
}

bool CheckUndefinedSymbols::visit(CastExpressionAST *ast)
{
    if (ast->lparen_token && ast->type_id && ast->rparen_token && ast->expression) {
        if (TypeIdAST *cast_type_id = ast->type_id->asTypeId()) {
            SpecifierListAST *type_specifier = cast_type_id->type_specifier_list;
            if (! cast_type_id->declarator && type_specifier && ! type_specifier->next &&
                type_specifier->value->asNamedTypeSpecifier() && ast->expression &&
                ast->expression->asUnaryExpression()) {
                // this ast node is ambigious, e.g.
                //   (a) + b
                // it can be parsed as
                //   ((a) + b)
                // or
                //   (a) (+b)
                accept(ast->expression);
                return false;
            }
        }
    }

    return true;
}

bool CheckUndefinedSymbols::visit(SizeofExpressionAST *ast)
{
    if (ast->lparen_token && ast->expression && ast->rparen_token) {
        if (TypeIdAST *type_id = ast->expression->asTypeId()) {
            SpecifierListAST *type_specifier = type_id->type_specifier_list;
            if (! type_id->declarator && type_specifier && ! type_specifier->next &&
                type_specifier->value->asNamedTypeSpecifier()) {
                // this sizeof expression is ambiguos, e.g.
                // sizeof (a)
                //   `a' can be a typeid or a nested-expression.
                return false;
            } else if (type_id->declarator
                       &&   type_id->declarator->postfix_declarator_list
                       && ! type_id->declarator->postfix_declarator_list->next
                       &&   type_id->declarator->postfix_declarator_list->value->asArrayDeclarator() != 0) {
                // this sizeof expression is ambiguos, e.g.
                // sizeof(a[10])
                //   `a' can be a typeid or an expression.
                return false;
            }
        }
    }

    return true;
}

bool CheckUndefinedSymbols::visit(ObjCClassDeclarationAST *ast)
{
    if (NameAST *nameAST = ast->superclass) {
        bool resolvedSuperClassName = false;

        if (const Name *name = nameAST->name) {
            const Identifier *id = name->identifier();
            const QByteArray spell = QByteArray::fromRawData(id->chars(), id->size());
            if (isType(spell))
                resolvedSuperClassName = true;
        }

        if (! resolvedSuperClassName) {
            translationUnit()->warning(nameAST->firstToken(),
                                       "expected class-name after ':' token");
        }
    }

    return true;
}

bool CheckUndefinedSymbols::visit(ObjCProtocolRefsAST *ast)
{
    for (NameListAST *iter = ast->identifier_list; iter; iter = iter->next) {
        if (NameAST *nameAST = iter->value) {
            bool resolvedProtocolName = false;

            if (const Name *name = nameAST->name) {
                const Identifier *id = name->identifier();
                const QByteArray spell = QByteArray::fromRawData(id->chars(), id->size());
                if (isProtocol(spell))
                    resolvedProtocolName = true;
            }

            if (!resolvedProtocolName) {
                char after;

                if (iter == ast->identifier_list)
                    after = '<';
                else
                    after = ',';

                translationUnit()->warning(nameAST->firstToken(), "expected protocol name after '%c' token", after);
            }
        }
    }

    return false;
}

bool CheckUndefinedSymbols::visit(ObjCPropertyDeclarationAST *ast)
{
    for (List<ObjCPropertyDeclaration *> *iter = ast->symbols; iter; iter = iter->next) {
        if (/*Name *getterName = */ iter->value->getterName()) {
            // FIXME: resolve the symbol for the name, and check its signature.
        }

        if (/*Name *setterName = */ iter->value->setterName()) {
            // FIXME: resolve the symbol for the name, and check its signature.
        }
    }

    return false;
}

bool CheckUndefinedSymbols::visit(QtDeclareFlagsDeclarationAST *ast)
{
    // ### check flags name too?

    if (ast->enum_name && ast->enum_name->name) {
        const Identifier *enumId = ast->enum_name->name->identifier();
        if (!isType(enumId)) // ### we're only checking if the enum name is known as a type name, not as an *enum*.
            translationUnit()->warning(ast->enum_name->firstToken(),
                                       "unknown enum '%s'",
                                       enumId->chars());
    }
    return false;
}

bool CheckUndefinedSymbols::visit(QtEnumDeclarationAST *ast)
{
    for (NameListAST *iter = ast->enumerator_list; iter; iter = iter->next) {
        if (! iter->value)
            continue;

        if (SimpleNameAST *enumName = iter->value->asSimpleName()) {
            if (enumName->name) {
                const Identifier *enumId = enumName->name->identifier();
                if (!isType(enumId))// ### we're only checking if the enum name is known as a type name, not as an *enum*.
                    translationUnit()->warning(enumName->firstToken(),
                                               "unknown enum '%s'",
                                               enumId->chars());
            }
        }
    }
    return false;
}

bool CheckUndefinedSymbols::visit(QtFlagsDeclarationAST *)
{
    // ### TODO
    return false;
}

bool CheckUndefinedSymbols::visit(QtPropertyDeclarationAST *)
{
    // ### TODO
    return false;
}
