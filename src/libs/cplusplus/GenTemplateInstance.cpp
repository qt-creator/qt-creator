
#include "GenTemplateInstance.h"
#include <Control.h>
#include <Scope.h>
#include <Names.h>
#include <Symbols.h>
#include <CoreTypes.h>
#include <QtCore/QVarLengthArray>

using namespace CPlusPlus;

GenTemplateInstance::GenTemplateInstance(Control *control, const Substitution &substitution)
    : _control(control),
      _substitution(substitution)
{ }

FullySpecifiedType GenTemplateInstance::operator()(const FullySpecifiedType &ty)
{ return subst(ty); }

FullySpecifiedType GenTemplateInstance::subst(Name *name)
{
    if (TemplateNameId *t = name->asTemplateNameId()) {
        QVarLengthArray<FullySpecifiedType, 8> args(t->templateArgumentCount());

        for (unsigned i = 0; i < t->templateArgumentCount(); ++i)
            args[i] = subst(t->templateArgumentAt(i));

        TemplateNameId *n = _control->templateNameId(t->identifier(),
                                                     args.data(), args.size());

        return FullySpecifiedType(_control->namedType(n));
    } else if (name->isQualifiedNameId()) {
        // ### implement me
    }

    for (int i = 0; i < _substitution.size(); ++i) {
        const QPair<Name *, FullySpecifiedType> s = _substitution.at(i);
        if (name->isEqualTo(s.first))
            return s.second;
    }

    return FullySpecifiedType(_control->namedType(name));
}

FullySpecifiedType GenTemplateInstance::subst(const FullySpecifiedType &ty)
{
    FullySpecifiedType previousType = switchType(ty);
    TypeVisitor::accept(ty.type());
    return switchType(previousType);
}

FullySpecifiedType GenTemplateInstance::switchType(const FullySpecifiedType &type)
{
    FullySpecifiedType previousType = _type;
    _type = type;
    return previousType;
}

// types
void GenTemplateInstance::visit(PointerToMemberType * /*ty*/)
{
    Q_ASSERT(false);
}

void GenTemplateInstance::visit(PointerType *ty)
{
    FullySpecifiedType elementType = subst(ty->elementType());
    _type.setType(_control->pointerType(elementType));
}

void GenTemplateInstance::visit(ReferenceType *ty)
{
    FullySpecifiedType elementType = subst(ty->elementType());
    _type.setType(_control->referenceType(elementType));
}

void GenTemplateInstance::visit(ArrayType *ty)
{
    FullySpecifiedType elementType = subst(ty->elementType());
    _type.setType(_control->arrayType(elementType, ty->size()));
}

void GenTemplateInstance::visit(NamedType *ty)
{
    Name *name = ty->name();
    _type.setType(subst(name).type());
}

void GenTemplateInstance::visit(Function *ty)
{
    Name *name = ty->name();
    FullySpecifiedType returnType = subst(ty->returnType());

    Function *fun = _control->newFunction(0, name);
    fun->setScope(ty->scope());
    fun->setConst(ty->isConst());
    fun->setVolatile(ty->isVolatile());
    fun->setReturnType(returnType);
    for (unsigned i = 0; i < ty->argumentCount(); ++i) {
        Symbol *arg = ty->argumentAt(i);
        FullySpecifiedType argTy = subst(arg->type());
        Argument *newArg = _control->newArgument(0, arg->name());
        newArg->setType(argTy);
        fun->arguments()->enterSymbol(newArg);
    }
    _type.setType(fun);
}

void GenTemplateInstance::visit(VoidType *)
{ /* nothing to do*/ }

void GenTemplateInstance::visit(IntegerType *)
{ /* nothing to do*/ }

void GenTemplateInstance::visit(FloatType *)
{ /* nothing to do*/ }

void GenTemplateInstance::visit(Namespace *)
{ Q_ASSERT(false); }

void GenTemplateInstance::visit(Class *)
{ Q_ASSERT(false); }

void GenTemplateInstance::visit(Enum *)
{ Q_ASSERT(false); }

// names
void GenTemplateInstance::visit(NameId *)
{ Q_ASSERT(false); }

void GenTemplateInstance::visit(TemplateNameId *)
{ Q_ASSERT(false); }

void GenTemplateInstance::visit(DestructorNameId *)
{ Q_ASSERT(false); }

void GenTemplateInstance::visit(OperatorNameId *)
{ Q_ASSERT(false); }

void GenTemplateInstance::visit(ConversionNameId *)
{ Q_ASSERT(false); }

void GenTemplateInstance::visit(QualifiedNameId *)
{ Q_ASSERT(false); }

