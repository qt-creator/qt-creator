
#include "TypeMatcher.h"
#include "CoreTypes.h"
#include "Symbols.h"
#include "Names.h"
#include "Literals.h"

using namespace CPlusPlus;

TypeMatcher::TypeMatcher()
{
}

TypeMatcher::~TypeMatcher()
{
}

bool TypeMatcher::isEqualTo(const Name *name, const Name *otherName) const
{
    if (name == otherName)
        return true;

    else if (! name || ! otherName)
        return false;

    return name->isEqualTo(otherName);
}

bool TypeMatcher::match(const UndefinedType *, const UndefinedType *)
{
    return true;
}

bool TypeMatcher::match(const VoidType *, const VoidType *)
{
    return true;
}

bool TypeMatcher::match(const IntegerType *type, const IntegerType *otherType)
{
    if (type == otherType)
        return true;

    else if (type->kind() != otherType->kind())
        return false;

    return true;
}

bool TypeMatcher::match(const FloatType *type, const FloatType *otherType)
{
    if (type == otherType)
        return true;

    else if (type->kind() != otherType->kind())
        return false;

    return true;
}

bool TypeMatcher::match(const PointerToMemberType *type, const PointerToMemberType *otherType)
{
    if (type == otherType)
        return true;

    else if (! isEqualTo(type->memberName(), otherType->memberName()))
        return false;

    else if (! type->elementType().match(otherType->elementType(), this))
        return false;

    return true;
}

bool TypeMatcher::match(const PointerType *type, const PointerType *otherType)
{
    if (type == otherType)
        return true;

    else if (! type->elementType().match(otherType->elementType(), this))
        return false;

    return true;
}

bool TypeMatcher::match(const ReferenceType *type, const ReferenceType *otherType)
{
    if (type == otherType)
        return true;

    else if (type->isRvalueReference() != otherType->isRvalueReference())
        return false;

    else if (! type->elementType().match(otherType->elementType(), this))
        return false;

    return true;
}

bool TypeMatcher::match(const ArrayType *type, const ArrayType *otherType)
{
    if (type == otherType)
        return true;

    else if (type->size() != otherType->size())
        return false;

    else if (! type->elementType().match(otherType->elementType(), this))
        return false;

    return true;
}

bool TypeMatcher::match(const NamedType *type, const NamedType *otherType)
{
    if (type == otherType)
        return true;

    else if (! isEqualTo(type->name(), otherType->name()))
        return false;

    return true;
}

bool TypeMatcher::match(const Function *type, const Function *otherType)
{
    if (type != otherType)
        return false;

    return true;
}

bool TypeMatcher::match(const Enum *type, const Enum *otherType)
{
    if (type != otherType)
        return false;

    return true;
}

bool TypeMatcher::match(const Namespace *type, const Namespace *otherType)
{
    if (type != otherType)
        return false;

    return true;
}

bool TypeMatcher::match(const Template *type, const Template *otherType)
{
    if (type != otherType)
        return false;

    return true;
}

bool TypeMatcher::match(const ForwardClassDeclaration *type, const ForwardClassDeclaration *otherType)
{
    if (type != otherType)
        return false;

    return true;
}

bool TypeMatcher::match(const Class *type, const Class *otherType)
{
    if (type != otherType)
        return false;

    return true;
}

bool TypeMatcher::match(const ObjCClass *type, const ObjCClass *otherType)
{
    if (type != otherType)
        return false;

    return true;
}

bool TypeMatcher::match(const ObjCProtocol *type, const ObjCProtocol *otherType)
{
    if (type != otherType)
        return false;

    return true;
}

bool TypeMatcher::match(const ObjCForwardClassDeclaration *type, const ObjCForwardClassDeclaration *otherType)
{
    if (type != otherType)
        return false;

    return true;
}

bool TypeMatcher::match(const ObjCForwardProtocolDeclaration *type, const ObjCForwardProtocolDeclaration *otherType)
{
    if (type != otherType)
        return false;

    return true;
}

bool TypeMatcher::match(const ObjCMethod *type, const ObjCMethod *otherType)
{
    if (type != otherType)
        return false;

    return true;
}
