/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "InsertionPointLocator.h"

#include <AST.h>
#include <ASTVisitor.h>
#include <TranslationUnit.h>

using namespace CPlusPlus;

namespace {

static QString generate(InsertionPointLocator::AccessSpec xsSpec)
{
    switch (xsSpec) {
    default:
    case InsertionPointLocator::Public:
        return QLatin1String("public:\n");

    case InsertionPointLocator::Protected:
        return QLatin1String("protected:\n");

    case InsertionPointLocator::Private:
        return QLatin1String("private:\n");

    case InsertionPointLocator::PublicSlot:
        return QLatin1String("public slots:\n");

    case InsertionPointLocator::ProtectedSlot:
        return QLatin1String("protected slots:\n");

    case InsertionPointLocator::PrivateSlot:
        return QLatin1String("private slot:\n");

    case InsertionPointLocator::Signals:
        return QLatin1String("signals:\n");
    }
}

static int distance(InsertionPointLocator::AccessSpec one, InsertionPointLocator::AccessSpec two)
{
    static QList<InsertionPointLocator::AccessSpec> distances = QList<InsertionPointLocator::AccessSpec>()
            << InsertionPointLocator::Public
            << InsertionPointLocator::PublicSlot
            << InsertionPointLocator::Signals
            << InsertionPointLocator::Protected
            << InsertionPointLocator::ProtectedSlot
            << InsertionPointLocator::PrivateSlot
            << InsertionPointLocator::Private
            ;

    return distances.indexOf(one) - distances.indexOf(two);
}

struct AccessRange
{
    unsigned start;
    unsigned end;
    InsertionPointLocator::AccessSpec xsSpec;

    AccessRange()
        : start(0)
        , end(0)
        , xsSpec(InsertionPointLocator::Invalid)
    {}

    AccessRange(unsigned start, unsigned end, InsertionPointLocator::AccessSpec xsSpec)
        : start(start)
        , end(end)
        , xsSpec(xsSpec)
    {}
};

class FindInClass: public ASTVisitor
{
public:
    FindInClass(const Document::Ptr &doc, const Class *clazz, InsertionPointLocator::AccessSpec xsSpec)
        : ASTVisitor(doc->translationUnit())
        , _doc(doc)
        , _clazz(clazz)
        , _xsSpec(xsSpec)
    {}

    InsertionLocation operator()()
    {
        _result = InsertionLocation();

        AST *ast = translationUnit()->ast();
        accept(ast);

        return _result;
    }

protected:
    using ASTVisitor::visit;

    bool visit(ClassSpecifierAST *ast)
    {
        if (!ast->lbrace_token || !ast->rbrace_token)
            return true;
        if (!ast->symbol || !ast->symbol->isEqualTo(_clazz))
            return true;

        QList<AccessRange> ranges = collectAccessRanges(
                    ast->member_specifier_list,
                    tokenKind(ast->classkey_token) == T_CLASS ? InsertionPointLocator::Private : InsertionPointLocator::Public,
                    ast->lbrace_token,
                    ast->rbrace_token);

        QPair<unsigned, bool> result = findMatch(ranges, _xsSpec);

        unsigned line = 0, column = 0;
        getTokenStartPosition(result.first, &line, &column);

        QString prefix;
        if (!result.second)
            prefix = generate(_xsSpec);

        _result = InsertionLocation(prefix, line, column);
        return false;
    }

    static QPair<unsigned, bool> findMatch(const QList<AccessRange> &ranges,
                                           InsertionPointLocator::AccessSpec xsSpec)
    {
        // try an exact match, and ignore the first (default) access spec:
        for (int i = ranges.size() - 1; i > 0; --i) {
            const AccessRange &range = ranges.at(i);
            if (range.xsSpec == xsSpec)
                return qMakePair(range.end, true);
        }

        // try to find a fitting access spec to insert in front of:
        AccessRange best = ranges.first();
        for (int i = ranges.size() - 1; i > 0; --i) {
            const AccessRange &range = ranges.at(i);
            if (distance(range.xsSpec, xsSpec) < distance(best.xsSpec, xsSpec))
                best = range;
        }

        // otherwise:
        return qMakePair(ranges.first().end, false);
    }

    QList<AccessRange> collectAccessRanges(DeclarationListAST *decls,
                                           InsertionPointLocator::AccessSpec initialXs,
                                           int firstRangeStart,
                                           int lastRangeEnd) const
    {
        QList<AccessRange> ranges;
        ranges.append(AccessRange(firstRangeStart, lastRangeEnd, initialXs));

        for (DeclarationListAST *iter = decls; iter; iter = iter->next) {
            DeclarationAST *decl = iter->value;

            if (AccessDeclarationAST *xsDecl = decl->asAccessDeclaration()) {
                const unsigned token = xsDecl->access_specifier_token;
                int newXsSpec = initialXs;
                bool isSlot = xsDecl->slots_token && tokenKind(xsDecl->slots_token) == T_Q_SLOTS;

                switch (tokenKind(token)) {
                case T_PUBLIC:
                    newXsSpec = isSlot ? InsertionPointLocator::PublicSlot : InsertionPointLocator::Public;
                    break;

                case T_PROTECTED:
                    newXsSpec = isSlot ? InsertionPointLocator::PublicSlot : InsertionPointLocator::Protected;
                    break;

                case T_PRIVATE:
                    newXsSpec = isSlot ? InsertionPointLocator::PublicSlot : InsertionPointLocator::Private;
                    break;

                case T_Q_SIGNALS:
                    newXsSpec = InsertionPointLocator::Signals;
                    break;

                case T_Q_SLOTS: {
                    newXsSpec = ranges.last().xsSpec | InsertionPointLocator::SlotBit;
                    break;
                }

                default:
                    break;
                }

                if (newXsSpec != ranges.last().xsSpec) {
                    ranges.last().end = token;
                    ranges.append(AccessRange(token, lastRangeEnd, (InsertionPointLocator::AccessSpec) newXsSpec));
                }
            }
        }

        ranges.last().end = lastRangeEnd;
        return ranges;
    }

private:
    Document::Ptr _doc;
    const Class *_clazz;
    InsertionPointLocator::AccessSpec _xsSpec;

    InsertionLocation _result;
};

} // end of anonymous namespace

InsertionLocation::InsertionLocation()
    : m_line(0)
    , m_column(0)
{}

InsertionLocation::InsertionLocation(const QString &prefix, unsigned line, unsigned column)
    : m_prefix(prefix)
    , m_line(line)
    , m_column(column)
{}

InsertionPointLocator::InsertionPointLocator(const Document::Ptr &doc)
    : m_doc(doc)
{
}

InsertionLocation InsertionPointLocator::methodDeclarationInClass(const Class *clazz, AccessSpec xsSpec) const
{
    FindInClass find(m_doc, clazz, xsSpec);
    return find();
}
