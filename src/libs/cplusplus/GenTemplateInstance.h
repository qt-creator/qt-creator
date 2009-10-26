#ifndef GENTEMPLATEINSTANCE_H
#define GENTEMPLATEINSTANCE_H

#include <TypeVisitor.h>
#include <NameVisitor.h>
#include <FullySpecifiedType.h>

#include <QtCore/QList>
#include <QtCore/QPair>

namespace CPlusPlus {

class CPLUSPLUS_EXPORT GenTemplateInstance: protected TypeVisitor, protected NameVisitor
{
public:
    typedef QList< QPair<Name *, FullySpecifiedType> > Substitution;

public:
    GenTemplateInstance(Control *control, const Substitution &substitution);

    FullySpecifiedType operator()(const FullySpecifiedType &ty);

protected:
    FullySpecifiedType subst(Name *name);
    FullySpecifiedType subst(const FullySpecifiedType &ty);

    FullySpecifiedType switchType(const FullySpecifiedType &type);

    virtual void visit(PointerToMemberType * /*ty*/);
    virtual void visit(PointerType *ty);
    virtual void visit(ReferenceType *ty);
    virtual void visit(ArrayType *ty);
    virtual void visit(NamedType *ty);
    virtual void visit(Function *ty);
    virtual void visit(VoidType *);
    virtual void visit(IntegerType *);
    virtual void visit(FloatType *);
    virtual void visit(Namespace *);
    virtual void visit(Class *);
    virtual void visit(Enum *);

    // names
    virtual void visit(NameId *);
    virtual void visit(TemplateNameId *);
    virtual void visit(DestructorNameId *);
    virtual void visit(OperatorNameId *);
    virtual void visit(ConversionNameId *);
    virtual void visit(QualifiedNameId *);

private:
    Control *_control;
    FullySpecifiedType _type;
    const Substitution _substitution;
};

} // end of namespace CPlusPlus

#endif // GENTEMPLATEINSTANCE_H
