/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "insertionpointlocator.h"

#include "cppprojectfile.h"
#include "cpptoolsreuse.h"
#include "symbolfinder.h"
#include "cpptoolsconstants.h"

#include <coreplugin/icore.h>

#include <cplusplus/LookupContext.h>

#include <utils/qtcassert.h>

using namespace CPlusPlus;
using namespace CppTools;

namespace {

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
    unsigned colonToken;

    AccessRange()
        : start(0)
        , end(0)
        , xsSpec(InsertionPointLocator::Invalid)
        , colonToken(0)
    {}

    AccessRange(unsigned start, unsigned end, InsertionPointLocator::AccessSpec xsSpec, unsigned colonToken)
        : start(start)
        , end(end)
        , xsSpec(xsSpec)
        , colonToken(colonToken)
    {}

    bool isEmpty() const
    {
        unsigned contentStart = 1 + (colonToken ? colonToken : start);
        return contentStart == end;
    }
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
        if (!ast->symbol || !ast->symbol->match(_clazz))
            return true;

        QList<AccessRange> ranges = collectAccessRanges(
                    ast->member_specifier_list,
                    tokenKind(ast->classkey_token) == T_CLASS ? InsertionPointLocator::Private : InsertionPointLocator::Public,
                    ast->lbrace_token,
                    ast->rbrace_token);

        unsigned beforeToken = 0;
        bool needsLeadingEmptyLine = false;
        bool needsPrefix = false;
        bool needsSuffix = false;
        findMatch(ranges, _xsSpec, beforeToken, needsLeadingEmptyLine, needsPrefix, needsSuffix);

        unsigned line = 0, column = 0;
        getTokenStartPosition(beforeToken, &line, &column);

        QString prefix;
        if (needsLeadingEmptyLine)
            prefix += QLatin1String("\n");
        if (needsPrefix)
            prefix += InsertionPointLocator::accessSpecToString(_xsSpec) + QLatin1String(":\n");

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
                          bool &needsLeadingEmptyLine,
                          bool &needsPrefix,
                          bool &needsSuffix)
    {
        QTC_ASSERT(!ranges.isEmpty(), return);
        const int lastIndex = ranges.size() - 1;

        needsLeadingEmptyLine = false;

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
        needsLeadingEmptyLine = !ranges.first().isEmpty();
        needsPrefix = true;
        needsSuffix = (ranges.size() != 1);
    }

    QList<AccessRange> collectAccessRanges(DeclarationListAST *decls,
                                           InsertionPointLocator::AccessSpec initialXs,
                                           int firstRangeStart,
                                           int lastRangeEnd) const
    {
        QList<AccessRange> ranges;
        ranges.append(AccessRange(firstRangeStart, lastRangeEnd, initialXs, 0));

        for (DeclarationListAST *iter = decls; iter; iter = iter->next) {
            DeclarationAST *decl = iter->value;

            if (AccessDeclarationAST *xsDecl = decl->asAccessDeclaration()) {
                const unsigned token = xsDecl->access_specifier_token;
                InsertionPointLocator::AccessSpec newXsSpec = initialXs;
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
                    newXsSpec = (InsertionPointLocator::AccessSpec)
                            (ranges.last().xsSpec | InsertionPointLocator::SlotBit);
                    break;
                }

                default:
                    break;
                }

                if (newXsSpec != ranges.last().xsSpec || ranges.size() == 1) {
                    ranges.last().end = token;
                    AccessRange r(token, lastRangeEnd, newXsSpec, xsDecl->colon_token);
                    ranges.append(r);
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

QString InsertionPointLocator::accessSpecToString(InsertionPointLocator::AccessSpec xsSpec)
{
    switch (xsSpec) {
    default:
    case InsertionPointLocator::Public:
        return QLatin1String("public");

    case InsertionPointLocator::Protected:
        return QLatin1String("protected");

    case InsertionPointLocator::Private:
        return QLatin1String("private");

    case InsertionPointLocator::PublicSlot:
        return QLatin1String("public slots");

    case InsertionPointLocator::ProtectedSlot:
        return QLatin1String("protected slots");

    case InsertionPointLocator::PrivateSlot:
        return QLatin1String("private slots");

    case InsertionPointLocator::Signals:
        return QLatin1String("signals");
    }
}

InsertionPointLocator::InsertionPointLocator(const CppRefactoringChanges &refactoringChanges)
    : m_refactoringChanges(refactoringChanges)
{
}

InsertionLocation InsertionPointLocator::methodDeclarationInClass(
    const QString &fileName,
    const Class *clazz,
    AccessSpec xsSpec) const
{
    const Document::Ptr doc = m_refactoringChanges.file(fileName)->cppDocument();
    if (doc) {
        FindInClass find(doc, clazz, xsSpec);
        return find();
    } else {
        return InsertionLocation();
    }
}

namespace {
template <class Key, class Value>
class HighestValue
{
    Key _key;
    Value _value;
    bool _set;
public:
    HighestValue()
        : _key(), _set(false)
    {}

    HighestValue(const Key &initialKey, const Value &initialValue)
        : _key(initialKey)
        , _value(initialValue)
        , _set(true)
    {}

    void maybeSet(const Key &key, const Value &value)
    {
        if (!_set || key > _key) {
            _value = value;
            _key = key;
            _set = true;
        }
    }

    const Value &get() const
    {
        QTC_CHECK(_set);
        return _value;
    }
};

class FindMethodDefinitionInsertPoint : protected ASTVisitor
{
    QList<const Identifier *> _namespaceNames;
    int _currentDepth = 0;
    HighestValue<int, unsigned> _bestToken;

public:
    FindMethodDefinitionInsertPoint(TranslationUnit *translationUnit)
        : ASTVisitor(translationUnit)
    {}

    void operator()(Symbol *decl, unsigned *line, unsigned *column)
    {
        // default to end of file
        const unsigned lastToken = translationUnit()->ast()->lastToken();
        _bestToken.maybeSet(-1, lastToken);

        if (lastToken >= 2) {
            QList<const Name *> names = LookupContext::fullyQualifiedName(decl);
            foreach (const Name *name, names) {
                const Identifier *id = name->asNameId();
                if (!id)
                    break;
                _namespaceNames += id;
            }
            _currentDepth = 0;

            accept(translationUnit()->ast());
        }

        if (lastToken == _bestToken.get()) // No matching namespace found
            translationUnit()->getTokenStartPosition(lastToken, line, column);
        else // Insert at end of matching namespace
            translationUnit()->getTokenEndPosition(_bestToken.get(), line, column);
    }

protected:
    bool preVisit(AST *ast)
    {
        return ast->asNamespace() || ast->asTranslationUnit() || ast->asLinkageBody();
    }

    bool visit(NamespaceAST *ast)
    {
        if (_currentDepth >= _namespaceNames.size())
            return false;

        // ignore anonymous namespaces
        if (!ast->identifier_token)
            return false;

        const Identifier *name = translationUnit()->identifier(ast->identifier_token);
        if (!name->equalTo(_namespaceNames.at(_currentDepth)))
            return false;

        // found a good namespace
        _bestToken.maybeSet(_currentDepth, ast->lastToken() - 2);

        ++_currentDepth;
        accept(ast->linkage_body);
        --_currentDepth;

        return false;
    }
};

class FindFunctionDefinition : protected ASTVisitor
{
    FunctionDefinitionAST *_result = nullptr;
    unsigned _line = 0;
    unsigned _column = 0;
public:
    FindFunctionDefinition(TranslationUnit *translationUnit)
        : ASTVisitor(translationUnit)
    {
    }

    FunctionDefinitionAST *operator()(unsigned line, unsigned column)
    {
        _result = nullptr;
        _line = line;
        _column = column;
        accept(translationUnit()->ast());
        return _result;
    }

protected:
    bool preVisit(AST *ast)
    {
        if (_result)
            return false;
        unsigned line, column;
        translationUnit()->getTokenStartPosition(ast->firstToken(), &line, &column);
        if (line > _line || (line == _line && column > _column))
            return false;
        translationUnit()->getTokenEndPosition(ast->lastToken() - 1, &line, &column);
        if (line < _line || (line == _line && column < _column))
            return false;
        return true;
    }

    bool visit(FunctionDefinitionAST *ast)
    {
        _result = ast;
        return false;
    }
};

} // anonymous namespace

static Declaration *isNonVirtualFunctionDeclaration(Symbol *s)
{
    if (!s)
        return 0;
    Declaration *declaration = s->asDeclaration();
    if (!declaration)
        return 0;
    Function *type = s->type()->asFunctionType();
    if (!type || type->isPureVirtual())
        return 0;
    return declaration;
}

static InsertionLocation nextToSurroundingDefinitions(Symbol *declaration,
                                                      const CppRefactoringChanges &changes,
                                                      const QString& destinationFile)
{
    InsertionLocation noResult;
    Class *klass = declaration->enclosingClass();
    if (!klass)
        return noResult;

    // find the index of declaration
    int declIndex = -1;
    for (unsigned i = 0; i < klass->memberCount(); ++i) {
        Symbol *s = klass->memberAt(i);
        if (s == declaration) {
            declIndex = i;
            break;
        }
    }
    if (declIndex == -1)
        return noResult;

    // scan preceding declarations for a function declaration (and see if it is defined)
    SymbolFinder symbolFinder;
    Function *definitionFunction = 0;
    QString prefix, suffix;
    Declaration *surroundingFunctionDecl = 0;
    for (int i = declIndex - 1; i >= 0; --i) {
        Symbol *s = klass->memberAt(i);
        if (s->isGenerated() || !(surroundingFunctionDecl = isNonVirtualFunctionDeclaration(s)))
            continue;
        if ((definitionFunction = symbolFinder.findMatchingDefinition(surroundingFunctionDecl,
                                                                      changes.snapshot())))
        {
            if (destinationFile.isEmpty() || destinationFile == QString::fromUtf8(
                        definitionFunction->fileName(), definitionFunction->fileNameLength())) {
                prefix = QLatin1String("\n\n");
                break;
            }
            definitionFunction = 0;
        }
    }
    if (!definitionFunction) {
        // try to find one below
        for (unsigned i = declIndex + 1; i < klass->memberCount(); ++i) {
            Symbol *s = klass->memberAt(i);
            surroundingFunctionDecl = isNonVirtualFunctionDeclaration(s);
            if (!surroundingFunctionDecl)
                continue;
            if ((definitionFunction = symbolFinder.findMatchingDefinition(surroundingFunctionDecl,
                                                                          changes.snapshot())))
            {
                if (destinationFile.isEmpty() || destinationFile == QString::fromUtf8(
                            definitionFunction->fileName(), definitionFunction->fileNameLength())) {
                    suffix = QLatin1String("\n\n");
                    break;
                }
                definitionFunction = 0;
            }
        }
    }

    if (!definitionFunction)
        return noResult;

    unsigned line, column;
    if (suffix.isEmpty()) {
        Document::Ptr targetDoc = changes.snapshot().document(QString::fromUtf8(definitionFunction->fileName()));
        if (!targetDoc)
            return noResult;

        targetDoc->translationUnit()->getPosition(definitionFunction->endOffset(), &line, &column);
    } else {
        // we don't have an offset to the start of the function definition, so we need to manually find it...
        CppRefactoringFilePtr targetFile = changes.file(QString::fromUtf8(definitionFunction->fileName()));
        if (!targetFile->isValid())
            return noResult;

        FindFunctionDefinition finder(targetFile->cppDocument()->translationUnit());
        FunctionDefinitionAST *functionDefinition = finder(definitionFunction->line(), definitionFunction->column());
        if (!functionDefinition)
            return noResult;

        targetFile->cppDocument()->translationUnit()->getTokenStartPosition(functionDefinition->firstToken(), &line, &column);
    }

    return InsertionLocation(QString::fromUtf8(definitionFunction->fileName()), prefix, suffix, line, column);
}

QList<InsertionLocation> InsertionPointLocator::methodDefinition(Symbol *declaration,
                                                                 bool useSymbolFinder,
                                                                 const QString &destinationFile) const
{
    QList<InsertionLocation> result;
    if (!declaration)
        return result;

    if (useSymbolFinder) {
        SymbolFinder symbolFinder;
        if (symbolFinder.findMatchingDefinition(declaration, m_refactoringChanges.snapshot(), true))
            return result;
    }

    const InsertionLocation location = nextToSurroundingDefinitions(declaration,
                                                                    m_refactoringChanges,
                                                                    destinationFile);
    if (location.isValid()) {
        result += location;
        return result;
    }

    const QString declFileName = QString::fromUtf8(declaration->fileName(),
                                                   declaration->fileNameLength());
    QString target = declFileName;
    if (!ProjectFile::isSource(ProjectFile::classify(declFileName))) {
        QString candidate = CppTools::correspondingHeaderOrSource(declFileName);
        if (!candidate.isEmpty())
            target = candidate;
    }

    CppRefactoringFilePtr targetFile = m_refactoringChanges.file(target);
    Document::Ptr doc = targetFile->cppDocument();
    if (doc.isNull())
        return result;

    unsigned line = 0, column = 0;
    FindMethodDefinitionInsertPoint finder(doc->translationUnit());
    finder(declaration, &line, &column);

    // Force empty lines before and after the new definition.
    QString prefix;
    QString suffix;
    if (!line) {
        // Totally empty file.
        line = 1;
        column = 1;
        prefix = suffix = QLatin1Char('\n');
    } else {
        QTC_ASSERT(column, return result);

        int firstNonSpace = targetFile->position(line, column);
        prefix = QLatin1String("\n\n");
        // Only one new line if at the end of file
        if (const QTextDocument *doc = targetFile->document()) {
            if (firstNonSpace + 1 == doc->characterCount() /* + 1 because zero based index */
                    && doc->characterAt(firstNonSpace) == QChar::ParagraphSeparator) {
                prefix = QLatin1String("\n");
            }
        }

        QChar c = targetFile->charAt(firstNonSpace);
        while (c == QLatin1Char(' ') || c == QLatin1Char('\t')) {
            ++firstNonSpace;
            c = targetFile->charAt(firstNonSpace);
        }
        if (targetFile->charAt(firstNonSpace) != QChar::ParagraphSeparator) {
            suffix.append(QLatin1String("\n\n"));
        } else {
            ++firstNonSpace;
            if (targetFile->charAt(firstNonSpace) != QChar::ParagraphSeparator)
                suffix.append(QLatin1Char('\n'));
        }
    }

    result += InsertionLocation(target, prefix, suffix, line, column);

    return result;
}
