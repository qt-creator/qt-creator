#ifndef GENTEMPLATEINSTANCE_H
#define GENTEMPLATEINSTANCE_H

#include <TypeVisitor.h>
#include <NameVisitor.h>
#include <FullySpecifiedType.h>

#include "LookupContext.h"

#include <QtCore/QList>
#include <QtCore/QPair>

namespace CPlusPlus {

class CPLUSPLUS_EXPORT GenTemplateInstance
{
public:
    typedef QList< QPair<Identifier *, FullySpecifiedType> > Substitution;

public:
    GenTemplateInstance(const LookupContext &context, const Substitution &substitution);

    FullySpecifiedType operator()(Symbol *symbol);

    Control *control() const;
    int findSubstitution(Identifier *id) const;

private:
    Symbol *_symbol;
    LookupContext _context;
    const Substitution _substitution;
};

} // end of namespace CPlusPlus

#endif // GENTEMPLATEINSTANCE_H
