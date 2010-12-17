/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "cpptoolsplugin.h"
#include "cpprefactoringchanges.h"
#include "insertionpointlocator.h"

#include <AST.h>
#include <ASTVisitor.h>
#include <TranslationUnit.h>

#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>

using namespace CPlusPlus;
using namespace CppTools;

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
        return QLatin1String("private slots:\n");

    case InsertionPointLocator::Signals:
        return QLatin1String("signals:\n");
    }
}

static int ordering(InsertionPointLocator::AccessSpec xsSpec)
{
    static QList<InsertionPointLocator::AccessSpec> order = QList<InsertionPointLocator::AccessSpec>()
            << InsertionPointLocator::Public
            << InsertionPointLocator::PublicSlot
            << InsertionPointLocator::Signals
            << InsertionPointLocator::Protected
            << InsertionPointLocator::ProtectedSlot
            << InsertionPointLocator::PrivateSlot
            << InsertionPointLocator::Private
            ;

    return order.indexOf(xsSpec);
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

        unsigned beforeToken = 0;
        bool needsPrefix = false;
        bool needsSuffix = false;
        findMatch(ranges, _xsSpec, beforeToken, needsPrefix, needsSuffix);

        unsigned line = 0, column = 0;
        getTokenStartPosition(beforeToken, &line, &column);

        QString prefix;
        if (needsPrefix)
            prefix = generate(_xsSpec);

        QString suffix;
        if (needsSuffix)
            suffix = QLatin1Char('\n');

        _result = InsertionLocation(_doc->fileName(), prefix, suffix,
                                    line, column);
        return false;
    }

    static void findMatch(const QList<AccessRange> &ranges,
                          InsertionPointLocator::AccessSpec xsSpec,
                          unsigned &beforeToken,
                          bool &needsPrefix,
                          bool &needsSuffix)
    {
        Q_ASSERT(!ranges.isEmpty());
        const int lastIndex = ranges.size() - 1;

        // try an exact match, and ignore the first (default) access spec:
        for (int i = lastIndex; i > 0; --i) {
            const AccessRange &range = ranges.at(i);
            if (range.xsSpec == xsSpec) {
                beforeToken = range.end;
                needsPrefix = false;
                needsSuffix = (i != lastIndex);
                return;
            }
        }

        // try to find a fitting access spec to insert XXX:
        for (int i = lastIndex; i > 0; --i) {
            const AccessRange &current = ranges.at(i);

            if (ordering(xsSpec) > ordering(current.xsSpec)) {
                beforeToken = current.end;
                needsPrefix = true;
                needsSuffix = (i != lastIndex);
                return;
            }
        }

        // otherwise:
        beforeToken = ranges.first().end;
        needsPrefix = true;
        needsSuffix = (ranges.size() != 1);
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
                bool isSlot = xsDecl->slots_token
                        && tokenKind(xsDecl->slots_token) == T_Q_SLOTS;

                switch (tokenKind(token)) {
                case T_PUBLIC:
                    newXsSpec = isSlot ? InsertionPointLocator::PublicSlot
                                       : InsertionPointLocator::Public;
                    break;

                case T_PROTECTED:
                    newXsSpec = isSlot ? InsertionPointLocator::ProtectedSlot
                                       : InsertionPointLocator::Protected;
                    break;

                case T_PRIVATE:
                    newXsSpec = isSlot ? InsertionPointLocator::PrivateSlot
                                       : InsertionPointLocator::Private;
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

InsertionLocation::InsertionLocation(const QString &fileName,
                                     const QString &prefix,
                                     const QString &suffix,
                                     unsigned line, unsigned column)
    : m_fileName(fileName)
    , m_prefix(prefix)
    , m_suffix(suffix)
    , m_line(line)
    , m_column(column)
{}

InsertionPointLocator::InsertionPointLocator(CppRefactoringChanges *refactoringChanges)
    : m_refactoringChanges(refactoringChanges)
{
}

InsertionLocation InsertionPointLocator::methodDeclarationInClass(
    const QString &fileName,
    const Class *clazz,
    AccessSpec xsSpec) const
{
    const Document::Ptr doc = m_refactoringChanges->file(fileName).cppDocument();
    if (doc) {
        FindInClass find(doc, clazz, xsSpec);
        return find();
    } else {
        return InsertionLocation();
    }
}

static bool isSourceFile(const QString &fileName)
{
    const Core::MimeDatabase *mimeDb = Core::ICore::instance()->mimeDatabase();
    Core::MimeType cSourceTy = mimeDb->findByType(QLatin1String("text/x-csrc"));
    Core::MimeType cppSourceTy = mimeDb->findByType(QLatin1String("text/x-c++src"));
    Core::MimeType mSourceTy = mimeDb->findByType(QLatin1String("text/x-objcsrc"));
    QStringList suffixes = cSourceTy.suffixes();
    suffixes += cppSourceTy.suffixes();
    suffixes += mSourceTy.suffixes();
    QFileInfo fileInfo(fileName);
    return suffixes.contains(fileInfo.suffix());
}

/// Currently, we return the end of fileName.cpp
/// \todo take the definitions of the surrounding declarations into account
QList<InsertionLocation> InsertionPointLocator::methodDefinition(
    Declaration *declaration) const
{
    QList<InsertionLocation> result;
    if (!declaration)
        return result;

    const QString declFileName = QString::fromUtf8(declaration->fileName(),
                                                   declaration->fileNameLength());
    QString target = declFileName;
    if (!isSourceFile(declFileName)) {
        Internal::CppToolsPlugin *cpptools = Internal::CppToolsPlugin::instance();
        QString candidate = cpptools->correspondingHeaderOrSource(declFileName);
        if (!candidate.isEmpty())
            target = candidate;
    }

    Document::Ptr doc = m_refactoringChanges->file(target).cppDocument();
    if (doc.isNull())
        return result;

    Snapshot simplified = m_refactoringChanges->snapshot().simplified(doc);
    if (Symbol *s = simplified.findMatchingDefinition(declaration)) {
        if (Function *f = s->asFunction()) {
            if (f->isConst() == declaration->type().isConst()
                    && f->isVolatile() == declaration->type().isVolatile())
                return result;
        }
    }

    TranslationUnit *xUnit = doc->translationUnit();
    unsigned tokenCount = xUnit->tokenCount();
    if (tokenCount < 2) // no tokens available
        return result;

    unsigned line = 0, column = 0;
    xUnit->getTokenEndPosition(xUnit->tokenCount() - 2, &line, &column);

    const QLatin1String prefix("\n\n");
    result.append(InsertionLocation(target, prefix, QString(), line, column));

    return result;
}
