/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/
#include "CppRewriter.h"
#include <TypeVisitor.h>
#include <NameVisitor.h>
#include <CoreTypes.h>
#include <Symbols.h>
#include <Literals.h>
#include <Names.h>
#include <Scope.h>
#include <Overview.h>

#include <QtCore/QVarLengthArray>
#include <QtCore/QRegExp>
#include <QtCore/QDebug>

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

            for (unsigned i = 0; i < type->argumentCount(); ++i) {
                Symbol *arg = type->argumentAt(i);

                Argument *newArg = control()->newArgument(0, 0);
                newArg->copy(arg);
                newArg->setName(rewrite->rewriteName(arg->name()));
                newArg->setType(rewrite->rewriteType(arg->type()));

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
            temps.append(control()->templateNameId(identifier(name->identifier()), args.data(), args.size()));
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


UseQualifiedNames::UseQualifiedNames()
{

}

UseQualifiedNames::~UseQualifiedNames()
{

}

FullySpecifiedType UseQualifiedNames::apply(const Name *name, Rewrite *rewrite) const
{
    SubstitutionEnvironment *env = rewrite->env;
    Scope *scope = env->scope();

    if (name->isQualifiedNameId() || name->isTemplateNameId())
        return FullySpecifiedType();

    if (! scope)
        return FullySpecifiedType();

    const LookupContext &context = env->context();
    Control *control = rewrite->control;

    const QList<LookupItem> results = context.lookup(name, scope);
    foreach (const LookupItem &r, results) {
        if (Symbol *d = r.declaration()) {
            const Name *n = 0;
            foreach (const Name *c,  LookupContext::fullyQualifiedName(d)) {
                if (! n)
                    n = c;
                else
                    n = control->qualifiedNameId(n, c);
            }

            return control->namedType(n);
        }

        return r.type();
    }

    return FullySpecifiedType();
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
    Q_ASSERT(re.isValid());
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
    if (type.startsWith("class ")) // MSVC prepends class,struct
        type.remove(0, 6);
    if (type.startsWith("struct "))
        type.remove(0, 7);

    type.replace(QLatin1Char('*'), QLatin1Char('@'));

    for (int i = 0; i < 10; ++i) {
        int start = type.indexOf("std::allocator<");
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
        if (inner == QLatin1String("char")) { // std::string
            simplifyStdString(QLatin1String("char"), QLatin1String("string"), &type);
        } else if (inner == QLatin1String("wchar_t")) { // std::wstring
            simplifyStdString(QLatin1String("wchar_t"), QLatin1String("wstring"), &type);
        } else if (inner == QLatin1String("unsigned short")) { // std::wstring/MSVC
            simplifyStdString(QLatin1String("unsigned short"), QLatin1String("wstring"), &type);
        }
        // std::vector, std::deque, std::list
        const QRegExp re1(QString::fromLatin1("(vector|list|deque)<%1, ?%2\\s*>").arg(inner, alloc));
        Q_ASSERT(re1.isValid());
        if (re1.indexIn(type) != -1)
            type.replace(re1.cap(0), QString::fromLatin1("%1<%2>").arg(re1.cap(1), inner));

        // std::stack
        QRegExp stackRE(QString::fromLatin1("stack<%1, ?std::deque<%2> >").arg(inner, inner));
        stackRE.setMinimal(true);
        Q_ASSERT(stackRE.isValid());
        if (stackRE.indexIn(type) != -1)
            type.replace(stackRE.cap(0), QString::fromLatin1("stack<%1>").arg(inner));

        // std::set
        QRegExp setRE(QString::fromLatin1("set<%1, ?std::less<%2>, ?%3\\s*>").arg(inner, inner, alloc));
        setRE.setMinimal(true);
        Q_ASSERT(setRE.isValid());
        if (setRE.indexIn(type) != -1)
            type.replace(setRE.cap(0), QString::fromLatin1("set<%1>").arg(inner));

        // std::map
        if (inner.startsWith("std::pair<")) {
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
            // Get value: MSVC: 'pair<a const ,b>', gcc: 'pair<const a, b>'
            if (inner.at(++pos) == QLatin1Char(' '))
                pos++;
            QString value = inner.mid(pos, inner.size() - pos - 1).trimmed();
            QRegExp mapRE1(QString("map<%1, ?%2, ?std::less<%3 ?>, ?%4\\s*>")
                .arg(key, value, key, alloc));
            mapRE1.setMinimal(true);
            Q_ASSERT(mapRE1.isValid());
            if (mapRE1.indexIn(type) != -1) {
                type.replace(mapRE1.cap(0), QString("map<%1, %2>").arg(key, value));
            } else {
                QRegExp mapRE2(QString("map<const %1, ?%2, ?std::less<const %3>, ?%4\\s*>")
                    .arg(key, value, key, alloc));
                mapRE2.setMinimal(true);
                if (mapRE2.indexIn(type) != -1) {
                    type.replace(mapRE2.cap(0), QString("map<const %1, %2>").arg(key, value));
                }
            }
        }
    }
    type.replace(QLatin1Char('@'), QLatin1Char('*'));
    type.replace(QLatin1String(" >"), QLatin1String(">"));
    return type;
}

} // namespace CPlusPlus
