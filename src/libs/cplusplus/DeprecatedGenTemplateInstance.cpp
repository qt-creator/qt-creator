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

#include "DeprecatedGenTemplateInstance.h"
#include "Overview.h"

#include <cplusplus/Control.h>
#include <cplusplus/Scope.h>
#include <cplusplus/Names.h>
#include <cplusplus/Symbols.h>
#include <cplusplus/CoreTypes.h>
#include <cplusplus/Literals.h>

#include <QVarLengthArray>
#include <QDebug>

using namespace CPlusPlus;

namespace {

class ApplySubstitution
{
public:
    ApplySubstitution(Control *control, Symbol *symbol, const DeprecatedGenTemplateInstance::Substitution &substitution);
    ~ApplySubstitution();

    inline Control *control() const { return _control; }

    FullySpecifiedType apply(const Name *name);
    FullySpecifiedType apply(const FullySpecifiedType &type);

    int findSubstitution(const Identifier *id) const;
    FullySpecifiedType applySubstitution(int index) const;

private:
    class ApplyToType: protected TypeVisitor
    {
    public:
        ApplyToType(ApplySubstitution *q)
            : q(q) {}

        FullySpecifiedType operator()(const FullySpecifiedType &ty)
        {
            FullySpecifiedType previousType = switchType(ty);
            accept(ty.type());
            return switchType(previousType);
        }

    protected:
        using TypeVisitor::visit;

        Control *control() const
        { return q->control(); }

        FullySpecifiedType switchType(const FullySpecifiedType &type)
        {
            FullySpecifiedType previousType = _type;
            _type = type;
            return previousType;
        }

        virtual void visit(VoidType *)
        {
            // nothing to do
        }

        virtual void visit(IntegerType *)
        {
            // nothing to do
        }

        virtual void visit(FloatType *)
        {
            // nothing to do
        }

        virtual void visit(PointerToMemberType *)
        {
            qDebug() << Q_FUNC_INFO; // ### TODO
        }

        virtual void visit(PointerType *ptrTy)
        {
            _type.setType(control()->pointerType(q->apply(ptrTy->elementType())));
        }

        virtual void visit(ReferenceType *refTy)
        {
            _type.setType(control()->referenceType(q->apply(refTy->elementType())));
        }

        virtual void visit(ArrayType *arrayTy)
        {
            _type.setType(control()->arrayType(q->apply(arrayTy->elementType()), arrayTy->size()));
        }

        virtual void visit(NamedType *ty)
        {
            FullySpecifiedType n = q->apply(ty->name());
            _type.setType(n.type());
        }

        virtual void visit(Function *funTy)
        {
            Function *fun = control()->newFunction(/*sourceLocation=*/ 0, funTy->name());
            fun->setScope(funTy->enclosingScope());
            fun->setConst(funTy->isConst());
            fun->setVolatile(funTy->isVolatile());
            fun->setVirtual(funTy->isVirtual());
            fun->setOverride(funTy->isOverride());
            fun->setFinal(funTy->isFinal());
            fun->setAmbiguous(funTy->isAmbiguous());
            fun->setVariadic(funTy->isVariadic());

            fun->setReturnType(q->apply(funTy->returnType()));

            for (unsigned i = 0, argc = funTy->argumentCount(); i < argc; ++i) {
                Argument *originalArgument = funTy->argumentAt(i)->asArgument();
                Argument *arg = control()->newArgument(/*sourceLocation*/ 0,
                                                       originalArgument->name());

                arg->setType(q->apply(originalArgument->type()));
                arg->setInitializer(originalArgument->initializer());
                fun->addMember(arg);
            }

            _type.setType(fun);
        }

        virtual void visit(Namespace *)
        {
            qDebug() << Q_FUNC_INFO;
        }

        virtual void visit(Class *)
        {
            qDebug() << Q_FUNC_INFO;
        }

        virtual void visit(Enum *)
        {
            qDebug() << Q_FUNC_INFO;
        }

        virtual void visit(ForwardClassDeclaration *)
        {
            qDebug() << Q_FUNC_INFO;
        }

        virtual void visit(ObjCClass *)
        {
            qDebug() << Q_FUNC_INFO;
        }

        virtual void visit(ObjCProtocol *)
        {
            qDebug() << Q_FUNC_INFO;
        }

        virtual void visit(ObjCMethod *)
        {
            qDebug() << Q_FUNC_INFO;
        }

        virtual void visit(ObjCForwardClassDeclaration *)
        {
            qDebug() << Q_FUNC_INFO;
        }

        virtual void visit(ObjCForwardProtocolDeclaration *)
        {
            qDebug() << Q_FUNC_INFO;
        }

    private:
        ApplySubstitution *q;
        FullySpecifiedType _type;
        QHash<Symbol *, FullySpecifiedType> _processed;
    };

    class ApplyToName: protected NameVisitor
    {
    public:
        ApplyToName(ApplySubstitution *q): q(q) {}

        FullySpecifiedType operator()(const Name *name)
        {
            FullySpecifiedType previousType = switchType(FullySpecifiedType());
            accept(name);
            return switchType(previousType);
        }

    protected:
        Control *control() const
        { return q->control(); }

        int findSubstitution(const Identifier *id) const
        { return q->findSubstitution(id); }

        FullySpecifiedType applySubstitution(int index) const
        { return q->applySubstitution(index); }

        FullySpecifiedType switchType(const FullySpecifiedType &type)
        {
            FullySpecifiedType previousType = _type;
            _type = type;
            return previousType;
        }

        virtual void visit(const Identifier *name)
        {
            int index = findSubstitution(name->identifier());

            if (index != -1)
                _type = applySubstitution(index);

            else
                _type = control()->namedType(name);
        }

        virtual void visit(const TemplateNameId *name)
        {
            QVarLengthArray<FullySpecifiedType, 8> arguments(name->templateArgumentCount());
            for (unsigned i = 0; i < name->templateArgumentCount(); ++i) {
                FullySpecifiedType argTy = name->templateArgumentAt(i);
                arguments[i] = q->apply(argTy);
            }

            const TemplateNameId *templId = control()->templateNameId(name->identifier(),
                                                                      name->isSpecialization(),
                                                                      arguments.data(),
                                                                      arguments.size());
            _type = control()->namedType(templId);
        }

        const Name *instantiate(const Name *name)
        {
            if (! name)
                return name;

            else if (const Identifier *nameId = name->asNameId()) {
                const Identifier *id = control()->identifier(nameId->chars(), nameId->size());
                return id;

            } else if (const TemplateNameId *templId = name->asTemplateNameId()) {
                QVarLengthArray<FullySpecifiedType, 8> arguments(templId->templateArgumentCount());
                for (unsigned templateArgIndex = 0; templateArgIndex < templId->templateArgumentCount();
                     ++templateArgIndex) {
                    FullySpecifiedType argTy = templId->templateArgumentAt(templateArgIndex);
                    arguments[templateArgIndex] = q->apply(argTy);
                }
                const Identifier *id = control()->identifier(templId->identifier()->chars(),
                                                                         templId->identifier()->size());
                return control()->templateNameId(id, templId->isSpecialization(), arguments.data(),
                                                 arguments.size());

            } else if (const QualifiedNameId *qq = name->asQualifiedNameId()) {
                const Name *base = instantiate(qq->base());
                const Name *name = instantiate(qq->name());

                return control()->qualifiedNameId(base, name);

            } else if (const OperatorNameId *op = name->asOperatorNameId()) {
                return control()->operatorNameId(op->kind());

            } else if (const ConversionNameId *c = name->asConversionNameId()) {
                FullySpecifiedType ty = q->apply(c->type());
                return control()->conversionNameId(ty);

            }

            return 0;
        }

        virtual void visit(const QualifiedNameId *name)
        {
            if (const Name *n = instantiate(name))
                _type = control()->namedType(n);
        }

        virtual void visit(const DestructorNameId *name)
        {
            Overview oo;
            qWarning() << "ignored name:" << oo.prettyName(name);
        }

        virtual void visit(const OperatorNameId *name)
        {
            Overview oo;
            qWarning() << "ignored name:" << oo.prettyName(name);
        }

        virtual void visit(const ConversionNameId *name)
        {
            Overview oo;
            qWarning() << "ignored name:" << oo.prettyName(name);
        }

        virtual void visit(const SelectorNameId *name)
        {
            Overview oo;
            qWarning() << "ignored name:" << oo.prettyName(name);
        }

    private:
        ApplySubstitution *q;
        FullySpecifiedType _type;
    };

public: // attributes
    Control *_control;
    Symbol *symbol;
    DeprecatedGenTemplateInstance::Substitution substitution;
    ApplyToType applyToType;
    ApplyToName applyToName;
};

ApplySubstitution::ApplySubstitution(Control *control, Symbol *symbol,
                                     const DeprecatedGenTemplateInstance::Substitution &substitution)
    : _control(control), symbol(symbol),
      substitution(substitution),
      applyToType(this), applyToName(this)
{ }

ApplySubstitution::~ApplySubstitution()
{
}

FullySpecifiedType ApplySubstitution::apply(const Name *name)
{
    FullySpecifiedType ty = applyToName(name);
    return ty;
}

FullySpecifiedType ApplySubstitution::apply(const FullySpecifiedType &type)
{
    FullySpecifiedType ty = applyToType(type);
    return ty;
}

int ApplySubstitution::findSubstitution(const Identifier *id) const
{
    Q_ASSERT(id != 0);

    for (int index = 0; index < substitution.size(); ++index) {
        QPair<const Identifier *, FullySpecifiedType> s = substitution.at(index);

        if (id->isEqualTo(s.first))
            return index;
    }

    return -1;
}

FullySpecifiedType ApplySubstitution::applySubstitution(int index) const
{
    Q_ASSERT(index != -1);
    Q_ASSERT(index < substitution.size());

    return substitution.at(index).second;
}

} // end of anonymous namespace

DeprecatedGenTemplateInstance::DeprecatedGenTemplateInstance(QSharedPointer<Control> control, const Substitution &substitution)
    : _control(control),
      _substitution(substitution)
{ }

FullySpecifiedType DeprecatedGenTemplateInstance::gen(Symbol *symbol)
{
    ApplySubstitution o(_control.data(), symbol, _substitution);
    return o.apply(symbol->type());
}

FullySpecifiedType DeprecatedGenTemplateInstance::instantiate(const Name *className, Symbol *candidate,
                                                              QSharedPointer<Control> control)
{
    if (className) {
        if (const TemplateNameId *templId = className->asTemplateNameId()) {
            if (Template *templ = candidate->enclosingTemplate()) {
                DeprecatedGenTemplateInstance::Substitution subst;

                for (unsigned i = 0; i < templId->templateArgumentCount(); ++i) {
                    FullySpecifiedType templArgTy = templId->templateArgumentAt(i);

                    if (i < templ->templateParameterCount()) {
                        const Name *templArgName = templ->templateParameterAt(i)->name();

                        if (templArgName && templArgName->identifier()) {
                            const Identifier *templArgId = templArgName->identifier();
                            subst.append(qMakePair(templArgId, templArgTy));
                        }
                    }
                }

                DeprecatedGenTemplateInstance inst(control, subst);
                return inst.gen(candidate);
            }
        }
    }
    return candidate->type();
}
