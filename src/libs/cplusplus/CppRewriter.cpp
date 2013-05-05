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

#include "CppRewriter.h"

#include "Overview.h"

#include <cplusplus/TypeVisitor.h>
#include <cplusplus/NameVisitor.h>
#include <cplusplus/CoreTypes.h>
#include <cplusplus/Symbols.h>
#include <cplusplus/Literals.h>
#include <cplusplus/Names.h>
#include <cplusplus/Scope.h>

#include <QVarLengthArray>
#include <QRegExp>
#include <QDebug>

#define QTC_ASSERT_STRINGIFY_HELPER(x) #x
#define QTC_ASSERT_STRINGIFY(x) QTC_ASSERT_STRINGIFY_HELPER(x)
#define QTC_ASSERT_STRING(cond) qDebug("SOFT ASSERT: \"" cond"\" in file " __FILE__ ", line " QTC_ASSERT_STRINGIFY(__LINE__))
#define QTC_ASSERT(cond, action) if (cond) {} else { QTC_ASSERT_STRING(#cond); action; } do {} while (0)

namespace CPlusPlus {

class Rewrite
{
public:
    Rewrite(Control *control, SubstitutionEnvironment *env)
        : control(control), env(env), rewriteType(this), rewriteName(this) {}

    class RewriteType: public TypeVisitor
    {
        Rewrite *rewrite;
        QList<FullySpecifiedType> temps;

        Control *control() const
        { return rewrite->control; }

        void accept(const FullySpecifiedType &ty)
        {
            TypeVisitor::accept(ty.type());
            unsigned flags = ty.flags();
            flags |= temps.back().flags();
            temps.back().setFlags(flags);
        }

    public:
        RewriteType(Rewrite *r): rewrite(r) {}

        FullySpecifiedType operator()(const FullySpecifiedType &ty)
        {
            accept(ty);
            return temps.takeLast();
        }

        virtual void visit(UndefinedType *)
        {
            temps.append(FullySpecifiedType());
        }

        virtual void visit(VoidType *)
        {
            temps.append(control()->voidType());
        }

        virtual void visit(IntegerType *type)
        {
            temps.append(control()->integerType(type->kind()));
        }

        virtual void visit(FloatType *type)
        {
            temps.append(control()->floatType(type->kind()));
        }

        virtual void visit(PointerToMemberType *type)
        {
            const Name *memberName = rewrite->rewriteName(type->memberName());
            const FullySpecifiedType elementType = rewrite->rewriteType(type->elementType());
            temps.append(control()->pointerToMemberType(memberName, elementType));
        }

        virtual void visit(PointerType *type)
        {
            const FullySpecifiedType elementType = rewrite->rewriteType(type->elementType());
            temps.append(control()->pointerType(elementType));
        }

        virtual void visit(ReferenceType *type)
        {
            const FullySpecifiedType elementType = rewrite->rewriteType(type->elementType());
            temps.append(control()->referenceType(elementType));
        }

        virtual void visit(ArrayType *type)
        {
            const FullySpecifiedType elementType = rewrite->rewriteType(type->elementType());
            temps.append(control()->arrayType(elementType, type->size()));
        }

        virtual void visit(NamedType *type)
        {
            FullySpecifiedType ty = rewrite->env->apply(type->name(), rewrite);
            if (! ty->isUndefinedType())
                temps.append(ty);
            else {
                const Name *name = rewrite->rewriteName(type->name());
                temps.append(control()->namedType(name));
            }
        }

        virtual void visit(Function *type)
        {
            Function *funTy = control()->newFunction(0, 0);
            funTy->copy(type);
            funTy->setConst(type->isConst());
            funTy->setVolatile(type->isVolatile());

            funTy->setName(rewrite->rewriteName(type->name()));

            funTy->setReturnType(rewrite->rewriteType(type->returnType()));

            for (unsigned i = 0, argc = type->argumentCount(); i < argc; ++i) {
                Symbol *arg = type->argumentAt(i);

                Argument *newArg = control()->newArgument(0, 0);
                newArg->copy(arg);
                newArg->setName(rewrite->rewriteName(arg->name()));
                newArg->setType(rewrite->rewriteType(arg->type()));

                // the copy() call above set the scope to 'type'
                // reset it to 0 before adding addMember to avoid assert
                newArg->resetScope();
                funTy->addMember(newArg);
            }

            temps.append(funTy);
        }

        virtual void visit(Namespace *type)
        {
            qWarning() << Q_FUNC_INFO;
            temps.append(type);
        }

        virtual void visit(Class *type)
        {
            qWarning() << Q_FUNC_INFO;
            temps.append(type);
        }

        virtual void visit(Enum *type)
        {
            qWarning() << Q_FUNC_INFO;
            temps.append(type);
        }

        virtual void visit(ForwardClassDeclaration *type)
        {
            qWarning() << Q_FUNC_INFO;
            temps.append(type);
        }

        virtual void visit(ObjCClass *type)
        {
            qWarning() << Q_FUNC_INFO;
            temps.append(type);
        }

        virtual void visit(ObjCProtocol *type)
        {
            qWarning() << Q_FUNC_INFO;
            temps.append(type);
        }

        virtual void visit(ObjCMethod *type)
        {
            qWarning() << Q_FUNC_INFO;
            temps.append(type);
        }

        virtual void visit(ObjCForwardClassDeclaration *type)
        {
            qWarning() << Q_FUNC_INFO;
            temps.append(type);
        }

        virtual void visit(ObjCForwardProtocolDeclaration *type)
        {
            qWarning() << Q_FUNC_INFO;
            temps.append(type);
        }

    };

    class RewriteName: public NameVisitor
    {
        Rewrite *rewrite;
        QList<const Name *> temps;

        Control *control() const
        { return rewrite->control; }

        const Identifier *identifier(const Identifier *other) const
        {
            if (! other)
                return 0;

            return control()->identifier(other->chars(), other->size());
        }

    public:
        RewriteName(Rewrite *r): rewrite(r) {}

        const Name *operator()(const Name *name)
        {
            if (! name)
                return 0;

            accept(name);
            return temps.takeLast();
        }

        virtual void visit(const QualifiedNameId *name)
        {
            const Name *base = rewrite->rewriteName(name->base());
            const Name *n = rewrite->rewriteName(name->name());
            temps.append(control()->qualifiedNameId(base, n));
        }

        virtual void visit(const Identifier *name)
        {
            temps.append(control()->identifier(name->chars(), name->size()));
        }

        virtual void visit(const TemplateNameId *name)
        {
            QVarLengthArray<FullySpecifiedType, 8> args(name->templateArgumentCount());
            for (unsigned i = 0; i < name->templateArgumentCount(); ++i)
                args[i] = rewrite->rewriteType(name->templateArgumentAt(i));
            temps.append(control()->templateNameId(identifier(name->identifier()), name->isSpecialization(),
                                                   args.data(), args.size()));
        }

        virtual void visit(const DestructorNameId *name)
        {
            temps.append(control()->destructorNameId(identifier(name->identifier())));
        }

        virtual void visit(const OperatorNameId *name)
        {
            temps.append(control()->operatorNameId(name->kind()));
        }

        virtual void visit(const ConversionNameId *name)
        {
            FullySpecifiedType ty = rewrite->rewriteType(name->type());
            temps.append(control()->conversionNameId(ty));
        }

        virtual void visit(const SelectorNameId *name)
        {
            QVarLengthArray<const Name *, 8> names(name->nameCount());
            for (unsigned i = 0; i < name->nameCount(); ++i)
                names[i] = rewrite->rewriteName(name->nameAt(i));
            temps.append(control()->selectorNameId(names.constData(), names.size(), name->hasArguments()));
        }
    };

public: // attributes
    Control *control;
    SubstitutionEnvironment *env;
    RewriteType rewriteType;
    RewriteName rewriteName;
};

SubstitutionEnvironment::SubstitutionEnvironment()
    : _scope(0)
{
}

FullySpecifiedType SubstitutionEnvironment::apply(const Name *name, Rewrite *rewrite) const
{
    if (name) {
        for (int index = _substs.size() - 1; index != -1; --index) {
            const Substitution *subst = _substs.at(index);

            FullySpecifiedType ty = subst->apply(name, rewrite);
            if (! ty->isUndefinedType())
                return ty;
        }
    }

    return FullySpecifiedType();
}

void SubstitutionEnvironment::enter(Substitution *subst)
{
    _substs.append(subst);
}

void SubstitutionEnvironment::leave()
{
    _substs.removeLast();
}

Scope *SubstitutionEnvironment::scope() const
{
    return _scope;
}

Scope *SubstitutionEnvironment::switchScope(Scope *scope)
{
    Scope *previous = _scope;
    _scope = scope;
    return previous;
}

const LookupContext &SubstitutionEnvironment::context() const
{
    return _context;
}

void SubstitutionEnvironment::setContext(const LookupContext &context)
{
    _context = context;
}

SubstitutionMap::SubstitutionMap()
{

}

SubstitutionMap::~SubstitutionMap()
{

}

void SubstitutionMap::bind(const Name *name, const FullySpecifiedType &ty)
{
    _map.append(qMakePair(name, ty));
}

FullySpecifiedType SubstitutionMap::apply(const Name *name, Rewrite *) const
{
    for (int n = _map.size() - 1; n != -1; --n) {
        const QPair<const Name *, FullySpecifiedType> &p = _map.at(n);

        if (name->isEqualTo(p.first))
            return p.second;
    }

    return FullySpecifiedType();
}


UseMinimalNames::UseMinimalNames(ClassOrNamespace *target)
    : _target(target)
{

}

UseMinimalNames::~UseMinimalNames()
{

}

FullySpecifiedType UseMinimalNames::apply(const Name *name, Rewrite *rewrite) const
{
    SubstitutionEnvironment *env = rewrite->env;
    Scope *scope = env->scope();

    if (name->isTemplateNameId() ||
            (name->isQualifiedNameId() && name->asQualifiedNameId()->name()->isTemplateNameId()))
        return FullySpecifiedType();

    if (! scope)
        return FullySpecifiedType();

    const LookupContext &context = env->context();
    Control *control = rewrite->control;

    const QList<LookupItem> results = context.lookup(name, scope);
    foreach (const LookupItem &r, results) {
        if (Symbol *d = r.declaration())
            return control->namedType(LookupContext::minimalName(d, _target, control));

        return r.type();
    }

    return FullySpecifiedType();
}


UseQualifiedNames::UseQualifiedNames()
    : UseMinimalNames(0)
{

}

UseQualifiedNames::~UseQualifiedNames()
{

}


FullySpecifiedType rewriteType(const FullySpecifiedType &type,
                               SubstitutionEnvironment *env,
                               Control *control)
{
    Rewrite rewrite(control, env);
    return rewrite.rewriteType(type);
}

const Name *rewriteName(const Name *name,
                        SubstitutionEnvironment *env,
                        Control *control)
{
    Rewrite rewrite(control, env);
    return rewrite.rewriteName(name);
}

// Simplify complicated STL template types,
// such as 'std::basic_string<char,std::char_traits<char>,std::allocator<char> >'
// -> 'std::string' and helpers.

static QString chopConst(QString type)
{
    while (1) {
        if (type.startsWith(QLatin1String("const")))
            type = type.mid(5);
        else if (type.startsWith(QLatin1Char(' ')))
            type = type.mid(1);
        else if (type.endsWith(QLatin1String("const")))
            type.chop(5);
        else if (type.endsWith(QLatin1Char(' ')))
            type.chop(1);
        else
            break;
    }
    return type;
}

static inline QRegExp stdStringRegExp(const QString &charType)
{
    QString rc = QLatin1String("basic_string<");
    rc += charType;
    rc += QLatin1String(",[ ]?std::char_traits<");
    rc += charType;
    rc += QLatin1String(">,[ ]?std::allocator<");
    rc += charType;
    rc += QLatin1String("> >");
    const QRegExp re(rc);
    QTC_ASSERT(re.isValid(), /**/);
    return re;
}

// Simplify string types in a type
// 'std::set<std::basic_string<char... > >' -> std::set<std::string>'
static inline void simplifyStdString(const QString &charType, const QString &replacement,
                                     QString *type)
{
    QRegExp stringRegexp = stdStringRegExp(charType);
    const int replacementSize = replacement.size();
    for (int pos = 0; pos < type->size(); ) {
        // Check next match
        const int matchPos = stringRegexp.indexIn(*type, pos);
        if (matchPos == -1)
            break;
        const int matchedLength = stringRegexp.matchedLength();
        type->replace(matchPos, matchedLength, replacement);
        pos = matchPos + replacementSize;
        // If we were inside an 'allocator<std::basic_string..char > >'
        // kill the following blank -> 'allocator<std::string>'
        if (pos + 1 < type->size() && type->at(pos) == QLatin1Char(' ')
                && type->at(pos + 1) == QLatin1Char('>'))
            type->remove(pos, 1);
    }
}

// Fix 'std::allocator<std::string >' -> 'std::allocator<std::string>',
// which can happen when replacing/simplifying
static inline QString fixNestedTemplates(QString s)
{
    const int size = s.size();
    if (size > 3
            && s.at(size - 1) == QLatin1Char('>')
            && s.at(size - 2) == QLatin1Char(' ')
            && s.at(size - 3) != QLatin1Char('>'))
        s.remove(size - 2, 1);
    return s;
}

CPLUSPLUS_EXPORT QString simplifySTLType(const QString &typeIn)
{
    QString type = typeIn;
    if (type.startsWith(QLatin1String("class "))) // MSVC prepends class,struct
        type.remove(0, 6);
    if (type.startsWith(QLatin1String("struct ")))
        type.remove(0, 7);

    type.replace(QLatin1Char('*'), QLatin1Char('@'));

    for (int i = 0; i < 10; ++i) {
        // std::ifstream
        QRegExp ifstreamRE(QLatin1String("std::basic_ifstream<char,\\s*std::char_traits<char>\\s*>"));
        ifstreamRE.setMinimal(true);
        QTC_ASSERT(ifstreamRE.isValid(), return typeIn);
        if (ifstreamRE.indexIn(type) != -1)
            type.replace(ifstreamRE.cap(0), QLatin1String("std::ifstream"));

        // Anything with a std::allocator
        int start = type.indexOf(QLatin1String("std::allocator<"));
        if (start == -1)
            break;
        // search for matching '>'
        int pos;
        int level = 0;
        for (pos = start + 12; pos < type.size(); ++pos) {
            int c = type.at(pos).unicode();
            if (c == '<') {
                ++level;
            } else if (c == '>') {
                --level;
                if (level == 0)
                    break;
            }
        }
        const QString alloc = fixNestedTemplates(type.mid(start, pos + 1 - start).trimmed());
        const QString inner = fixNestedTemplates(alloc.mid(15, alloc.size() - 16).trimmed());
        const QString allocEsc = QRegExp::escape(alloc);
        const QString innerEsc = QRegExp::escape(inner);
        if (inner == QLatin1String("char")) { // std::string
            simplifyStdString(QLatin1String("char"), QLatin1String("string"), &type);
        } else if (inner == QLatin1String("wchar_t")) { // std::wstring
            simplifyStdString(QLatin1String("wchar_t"), QLatin1String("wstring"), &type);
        } else if (inner == QLatin1String("unsigned short")) { // std::wstring/MSVC
            simplifyStdString(QLatin1String("unsigned short"), QLatin1String("wstring"), &type);
        }
        // std::vector, std::deque, std::list
        QRegExp re1(QString::fromLatin1("(vector|list|deque)<%1, ?%2\\s*>").arg(innerEsc, allocEsc));
        QTC_ASSERT(re1.isValid(), return typeIn);
        if (re1.indexIn(type) != -1)
            type.replace(re1.cap(0), QString::fromLatin1("%1<%2>").arg(re1.cap(1), inner));

        // std::stack
        QRegExp stackRE(QString::fromLatin1("stack<%1, ?std::deque<%2> >").arg(innerEsc, innerEsc));
        stackRE.setMinimal(true);
        QTC_ASSERT(stackRE.isValid(), return typeIn);
        if (stackRE.indexIn(type) != -1)
            type.replace(stackRE.cap(0), QString::fromLatin1("stack<%1>").arg(inner));

        // std::set
        QRegExp setRE(QString::fromLatin1("set<%1, ?std::less<%2>, ?%3\\s*>").arg(innerEsc, innerEsc, allocEsc));
        setRE.setMinimal(true);
        QTC_ASSERT(setRE.isValid(), return typeIn);
        if (setRE.indexIn(type) != -1)
            type.replace(setRE.cap(0), QString::fromLatin1("set<%1>").arg(inner));

        // std::map
        if (inner.startsWith(QLatin1String("std::pair<"))) {
            // search for outermost ',', split key and value
            int pos;
            int level = 0;
            for (pos = 10; pos < inner.size(); ++pos) {
                int c = inner.at(pos).unicode();
                if (c == '<')
                    ++level;
                else if (c == '>')
                    --level;
                else if (c == ',' && level == 0)
                    break;
            }
            const QString key = chopConst(inner.mid(10, pos - 10));
            const QString keyEsc = QRegExp::escape(key);
            // Get value: MSVC: 'pair<a const ,b>', gcc: 'pair<const a, b>'
            if (inner.at(++pos) == QLatin1Char(' '))
                pos++;
            const QString value = inner.mid(pos, inner.size() - pos - 1).trimmed();
            const QString valueEsc = QRegExp::escape(value);
            QRegExp mapRE1(QString::fromLatin1("map<%1, ?%2, ?std::less<%3 ?>, ?%4\\s*>")
                           .arg(keyEsc, valueEsc, keyEsc, allocEsc));
            mapRE1.setMinimal(true);
            QTC_ASSERT(mapRE1.isValid(), return typeIn);
            if (mapRE1.indexIn(type) != -1) {
                type.replace(mapRE1.cap(0), QString::fromLatin1("map<%1, %2>").arg(key, value));
            } else {
                QRegExp mapRE2(QString::fromLatin1("map<const %1, ?%2, ?std::less<const %3>, ?%4\\s*>")
                               .arg(keyEsc, valueEsc, keyEsc, allocEsc));
                mapRE2.setMinimal(true);
                if (mapRE2.indexIn(type) != -1)
                    type.replace(mapRE2.cap(0), QString::fromLatin1("map<const %1, %2>").arg(key, value));
            }
        }
    }
    type.replace(QLatin1Char('@'), QLatin1Char('*'));
    type.replace(QLatin1String(" >"), QLatin1String(">"));
    return type;
}

} // namespace CPlusPlus
