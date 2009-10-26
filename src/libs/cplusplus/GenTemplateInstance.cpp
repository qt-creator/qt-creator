
#include "GenTemplateInstance.h"
#include "Overview.h"

#include <Control.h>
#include <Scope.h>
#include <Names.h>
#include <Symbols.h>
#include <CoreTypes.h>
#include <Literals.h>

#include <QtCore/QVarLengthArray>
#include <QtCore/QDebug>

using namespace CPlusPlus;

namespace {

class ApplySubstitution
{
public:
    ApplySubstitution(const LookupContext &context, const GenTemplateInstance::Substitution &substitution);
    ~ApplySubstitution();

    Control *control() const { return context.control(); }

    FullySpecifiedType operator()(Name *name);
    FullySpecifiedType operator()(const FullySpecifiedType &type);

    int findSubstitution(Identifier *id) const;
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
            _type.setType(control()->pointerType(operator()(ptrTy->elementType())));
        }

        virtual void visit(ReferenceType *refTy)
        {
            _type.setType(control()->referenceType(operator()(refTy->elementType())));
        }

        virtual void visit(ArrayType *arrayTy)
        {
            _type.setType(control()->arrayType(operator()(arrayTy->elementType()), arrayTy->size()));
        }

        virtual void visit(NamedType *ty)
        {
            FullySpecifiedType n = q->operator ()(ty->name());
            _type.setType(n.type());
        }

        virtual void visit(Function *funTy)
        {
            Function *fun = control()->newFunction(/*sourceLocation=*/ 0, funTy->name());
            fun->setScope(funTy->scope());
            fun->setConst(funTy->isConst());
            fun->setVolatile(funTy->isVolatile());
            fun->setVirtual(funTy->isVirtual());
            fun->setAmbiguous(funTy->isAmbiguous());
            fun->setVariadic(funTy->isVariadic());

            fun->setReturnType(q->operator ()(funTy->returnType()));

            for (unsigned i = 0; i < funTy->argumentCount(); ++i) {
                Argument *originalArgument = funTy->argumentAt(i)->asArgument();
                Argument *arg = control()->newArgument(/*sourceLocation*/ 0,
                                                       originalArgument->name());

                arg->setType(q->operator ()(originalArgument->type()));
                arg->setInitializer(originalArgument->hasInitializer());
                fun->arguments()->enterSymbol(arg);
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
    };

    class ApplyToName: protected NameVisitor
    {
    public:
        ApplyToName(ApplySubstitution *q): q(q) {}

        FullySpecifiedType operator()(Name *name)
        {
            FullySpecifiedType previousType = switchType(FullySpecifiedType());
            accept(name);
            return switchType(previousType);
        }

    protected:
        Control *control() const
        { return q->control(); }

        int findSubstitution(Identifier *id) const
        { return q->findSubstitution(id); }

        FullySpecifiedType applySubstitution(int index) const
        { return q->applySubstitution(index); }

        FullySpecifiedType switchType(const FullySpecifiedType &type)
        {
            FullySpecifiedType previousType = _type;
            _type = type;
            return previousType;
        }

        virtual void visit(NameId *name)
        {
            int index = findSubstitution(name->identifier());

            if (index != -1)
                _type = applySubstitution(index);

            else
                _type = control()->namedType(name);
        }

        virtual void visit(TemplateNameId *name)
        {
            QVarLengthArray<FullySpecifiedType, 8> arguments(name->templateArgumentCount());
            for (unsigned i = 0; i < name->templateArgumentCount(); ++i) {
                FullySpecifiedType argTy = name->templateArgumentAt(i);
                arguments[i] = q->operator ()(argTy);
            }

            TemplateNameId *templId = control()->templateNameId(name->identifier(), arguments.data(), arguments.size());
            _type = control()->namedType(templId);
        }

        virtual void visit(QualifiedNameId *name)
        {
            QVarLengthArray<Name *, 8> names(name->nameCount());
            for (unsigned i = 0; i < name->nameCount(); ++i) {
                Name *n = name->nameAt(i);

                if (TemplateNameId *templId = n->asTemplateNameId()) {
                    QVarLengthArray<FullySpecifiedType, 8> arguments(templId->templateArgumentCount());
                    for (unsigned templateArgIndex = 0; templateArgIndex < templId->templateArgumentCount(); ++templateArgIndex) {
                        FullySpecifiedType argTy = templId->templateArgumentAt(templateArgIndex);
                        arguments[templateArgIndex] = q->operator ()(argTy);
                    }

                    n = control()->templateNameId(templId->identifier(), arguments.data(), arguments.size());
                }

                names[i] = n;
            }

            QualifiedNameId *q = control()->qualifiedNameId(names.data(), names.size(), name->isGlobal());
            _type = control()->namedType(q);
        }

        virtual void visit(DestructorNameId *name)
        {
            Overview oo;
            qWarning() << "ignored name:" << oo(name);
        }

        virtual void visit(OperatorNameId *name)
        {
            Overview oo;
            qWarning() << "ignored name:" << oo(name);
        }

        virtual void visit(ConversionNameId *name)
        {
            Overview oo;
            qWarning() << "ignored name:" << oo(name);
        }

        virtual void visit(SelectorNameId *name)
        {
            Overview oo;
            qWarning() << "ignored name:" << oo(name);
        }

    private:
        ApplySubstitution *q;
        FullySpecifiedType _type;
    };

public: // attributes
    LookupContext context;
    GenTemplateInstance::Substitution substitution;

private:
    ApplyToType applyToType;
    ApplyToName applyToName;
};

ApplySubstitution::ApplySubstitution(const LookupContext &context, const GenTemplateInstance::Substitution &substitution)
    : context(context), substitution(substitution), applyToType(this), applyToName(this)
{ }

ApplySubstitution::~ApplySubstitution()
{
}

FullySpecifiedType ApplySubstitution::operator()(Name *name)
{ return applyToName(name); }

FullySpecifiedType ApplySubstitution::operator()(const FullySpecifiedType &type)
{ return applyToType(type); }

int ApplySubstitution::findSubstitution(Identifier *id) const
{
    Q_ASSERT(id != 0);

    for (int index = 0; index < substitution.size(); ++index) {
        QPair<Identifier *, FullySpecifiedType> s = substitution.at(index);

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

GenTemplateInstance::GenTemplateInstance(const LookupContext &context, const Substitution &substitution)
    : _symbol(0),
      _context(context),
      _substitution(substitution)
{ }

FullySpecifiedType GenTemplateInstance::operator()(Symbol *symbol)
{
    ApplySubstitution o(_context, _substitution);
    return o(symbol->type());
}

Control *GenTemplateInstance::control() const
{ return _context.control(); }

int GenTemplateInstance::findSubstitution(Identifier *id) const
{
    int index = 0;

    for (; index < _substitution.size(); ++index) {
        const QPair<Identifier *, FullySpecifiedType> s = _substitution.at(index);

        if (id->isEqualTo(s.first))
            break;
    }

    return index;
}
