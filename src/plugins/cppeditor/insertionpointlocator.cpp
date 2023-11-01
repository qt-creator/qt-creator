// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "insertionpointlocator.h"

#include "cppprojectfile.h"
#include "cpptoolsreuse.h"
#include "symbolfinder.h"

#include <coreplugin/icore.h>

#include <cplusplus/ASTPath.h>
#include <cplusplus/LookupContext.h>

#include <utils/qtcassert.h>

using namespace CPlusPlus;
using namespace Utils;

namespace CppEditor {
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
    int start = 0;
    int beforeStart = 0;
    unsigned end = 0;
    InsertionPointLocator::AccessSpec xsSpec = InsertionPointLocator::Invalid;
    unsigned colonToken = 0;

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
    FindInClass(TranslationUnit *tu, const Class *clazz)
        : ASTVisitor(tu)
        , _clazz(clazz)
    {}

    ClassSpecifierAST *operator()()
    {
        _result = nullptr;

        AST *ast = translationUnit()->ast();
        accept(ast);

        return _result;
    }

protected:
    using ASTVisitor::visit;

    bool visit(ClassSpecifierAST *ast) override
    {
        if (!ast->lbrace_token || !ast->rbrace_token)
            return true;
        if (!ast->symbol || !ast->symbol->match(_clazz))
            return true;
        _result = ast;
        return false;
    }

private:
    const Class * const _clazz;
    ClassSpecifierAST *_result = nullptr;
};

void findMatch(const QList<AccessRange> &ranges,
               InsertionPointLocator::AccessSpec xsSpec,
               InsertionPointLocator::Position positionInAccessSpec,
               InsertionPointLocator::ForceAccessSpec forceAccessSpec,
               unsigned &beforeToken,
               bool &needsLeadingEmptyLine,
               bool &needsPrefix,
               bool &needsSuffix)
{
    QTC_ASSERT(!ranges.isEmpty(), return );
    const int lastIndex = ranges.size() - 1;
    const bool atEnd = positionInAccessSpec == InsertionPointLocator::AccessSpecEnd;
    needsLeadingEmptyLine = false;

    // Try an exact match. Ignore the default access spec unless there is no explicit one.
    const int firstIndex = ranges.size() == 1
            && forceAccessSpec == InsertionPointLocator::ForceAccessSpec::No ? 0 : 1;
    for (int i = lastIndex; i >= firstIndex; --i) {
        const AccessRange &range = ranges.at(i);
        if (range.xsSpec == xsSpec) {
            beforeToken = atEnd ? range.end : range.beforeStart;
            needsLeadingEmptyLine = !atEnd;
            needsPrefix = false;
            needsSuffix = (i != lastIndex);
            return;
        }
    }

    // try to find a fitting access spec to insert XXX:
    for (int i = lastIndex; i > 0; --i) {
        const AccessRange &current = ranges.at(i);

        if (ordering(xsSpec) > ordering(current.xsSpec)) {
            beforeToken = atEnd ? current.end : current.end - 1;
            needsLeadingEmptyLine = !atEnd;
            needsPrefix = true;
            needsSuffix = (i != lastIndex);
            return;
        }
    }

    // otherwise:
    beforeToken = atEnd ? ranges.first().end : ranges.first().end - 1;
    needsLeadingEmptyLine = !ranges.first().isEmpty();
    needsPrefix = true;
    needsSuffix = (ranges.size() != 1);
}

QList<AccessRange> collectAccessRanges(const CPlusPlus::TranslationUnit *tu,
                                       DeclarationListAST *decls,
                                       InsertionPointLocator::AccessSpec initialXs,
                                       int firstRangeStart,
                                       int lastRangeEnd)
{
    QList<AccessRange> ranges;
    ranges.append(AccessRange(firstRangeStart, lastRangeEnd, initialXs, 0));

    for (DeclarationListAST *iter = decls; iter; iter = iter->next) {
        DeclarationAST *decl = iter->value;

        if (AccessDeclarationAST *xsDecl = decl->asAccessDeclaration()) {
            const unsigned token = xsDecl->access_specifier_token;
            if (tu->tokenAt(token).generated())
                continue;
            InsertionPointLocator::AccessSpec newXsSpec = initialXs;
            bool isSlot = xsDecl->slots_token && tu->tokenKind(xsDecl->slots_token) == T_Q_SLOTS;

            switch (tu->tokenKind(token)) {
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
                newXsSpec = (InsertionPointLocator::AccessSpec)(ranges.last().xsSpec
                                                                | InsertionPointLocator::SlotBit);
                break;
            }

            default:
                break;
            }

            if (newXsSpec != ranges.last().xsSpec || ranges.size() == 1) {
                ranges.last().end = token;
                AccessRange r(token, lastRangeEnd, newXsSpec, xsDecl->colon_token);
                r.beforeStart = xsDecl->lastToken() - 1;
                ranges.append(r);
            }
        }
    }

    ranges.last().end = lastRangeEnd;
    return ranges;
}

} // end of anonymous namespace

InsertionLocation::InsertionLocation() = default;

InsertionLocation::InsertionLocation(const FilePath &filePath,
                                     const QString &prefix,
                                     const QString &suffix,
                                     int line, int column)
    : m_filePath(filePath)
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
    const Utils::FilePath &filePath,
    const Class *clazz,
    AccessSpec xsSpec,
    ForceAccessSpec forceAccessSpec) const
{
    const Document::Ptr doc = m_refactoringChanges.file(filePath)->cppDocument();
    if (doc) {
        FindInClass find(doc->translationUnit(), clazz);
        ClassSpecifierAST *classAST = find();
        return methodDeclarationInClass(doc->translationUnit(), classAST, xsSpec, AccessSpecEnd,
                                        forceAccessSpec);
    } else {
        return InsertionLocation();
    }
}

InsertionLocation InsertionPointLocator::methodDeclarationInClass(const TranslationUnit *tu,
    const ClassSpecifierAST *clazz,
    InsertionPointLocator::AccessSpec xsSpec,
    Position pos,
    ForceAccessSpec forceAccessSpec) const
{
    if (!clazz)
        return {};
    QList<AccessRange> ranges = collectAccessRanges(tu,
                                                    clazz->member_specifier_list,
                                                    tu->tokenKind(clazz->classkey_token) == T_CLASS
                                                        ? InsertionPointLocator::Private
                                                        : InsertionPointLocator::Public,
                                                    clazz->lbrace_token,
                                                    clazz->rbrace_token);

    unsigned beforeToken = 0;
    bool needsLeadingEmptyLine = false;
    bool needsPrefix = false;
    bool needsSuffix = false;
    findMatch(ranges, xsSpec, pos, forceAccessSpec, beforeToken, needsLeadingEmptyLine,
              needsPrefix, needsSuffix);

    int line = 0, column = 0;
    if (pos == InsertionPointLocator::AccessSpecEnd)
        tu->getTokenPosition(beforeToken, &line, &column);
    else
        tu->getTokenEndPosition(beforeToken, &line, &column);

    QString prefix;
    if (needsLeadingEmptyLine)
        prefix += QLatin1String("\n");
    if (needsPrefix)
        prefix += InsertionPointLocator::accessSpecToString(xsSpec) + QLatin1String(":\n");

    QString suffix;
    if (needsSuffix)
        suffix = QLatin1Char('\n');
    const QString fileName = QString::fromUtf8(tu->fileName(), tu->fileNameLength());
    return InsertionLocation(FilePath::fromString(fileName), prefix, suffix, line, column);
}

InsertionPointLocator::AccessSpec symbolsAccessSpec(Symbol *symbol)
{
    if (symbol->isPrivate())
        return InsertionPointLocator::Private;
    if (symbol->isProtected())
        return InsertionPointLocator::Protected;
    if (symbol->isPublic())
        return InsertionPointLocator::Public;
    return InsertionPointLocator::Invalid;
}

InsertionLocation InsertionPointLocator::constructorDeclarationInClass(
    const CPlusPlus::TranslationUnit *tu,
    const ClassSpecifierAST *clazz,
    InsertionPointLocator::AccessSpec xsSpec,
    int constructorArgumentCount) const
{
    std::map<int, std::pair<DeclarationAST *, DeclarationAST *>> constructors;
    for (DeclarationAST *rootDecl : clazz->member_specifier_list) {
        if (SimpleDeclarationAST *ast = rootDecl->asSimpleDeclaration()) {
            if (!ast->symbols)
                continue;
            if (symbolsAccessSpec(ast->symbols->value) != xsSpec)
                continue;
            if (ast->symbols->value->name() != clazz->name->name)
                continue;
            for (DeclaratorAST *d : ast->declarator_list) {
                for (PostfixDeclaratorAST *decl : d->postfix_declarator_list) {
                    if (FunctionDeclaratorAST *func = decl->asFunctionDeclarator()) {
                        int params = 0;
                        if (func->parameter_declaration_clause) {
                            params = size(
                                func->parameter_declaration_clause->parameter_declaration_list);
                        }
                        auto &entry = constructors[params];
                        if (!entry.first)
                            entry.first = rootDecl;
                        entry.second = rootDecl;
                    }
                }
            }
        }
    }
    if (constructors.empty())
        return methodDeclarationInClass(tu, clazz, xsSpec, AccessSpecBegin);

    auto iter = constructors.lower_bound(constructorArgumentCount);
    if (iter == constructors.end()) {
        // we have a constructor with x arguments but there are only ones with < x arguments
        --iter; // select greatest one (in terms of argument count)
    }
    const FilePath filePath =
            FilePath::fromString(QString::fromUtf8(tu->fileName(), tu->fileNameLength()));
    int line, column;
    if (iter->first <= constructorArgumentCount) {
        tu->getTokenEndPosition(iter->second.second->lastToken() - 1, &line, &column);
        return InsertionLocation(filePath, "\n", "", line, column);
    }
    // before iter
    // end pos of firstToken-1 instead of start pos of firstToken to skip leading commend
    tu->getTokenEndPosition(iter->second.first->firstToken() - 1, &line, &column);
    return InsertionLocation(filePath, "\n", "", line, column);
}

namespace {
template <class Key, class Value>
class HighestValue
{
    Key _key{};
    Value _value{};
    bool _set = false;
public:
    HighestValue() = default;

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
    explicit FindMethodDefinitionInsertPoint(TranslationUnit *translationUnit)
        : ASTVisitor(translationUnit)
    {}

    void operator()(Symbol *decl, int *line, int *column)
    {
        // default to end of file
        const unsigned lastToken = translationUnit()->ast()->lastToken();
        _bestToken.maybeSet(-1, lastToken);

        if (lastToken >= 2) {
            const QList<const Name *> names = LookupContext::fullyQualifiedName(decl);
            for (const Name *name : names) {
                const Identifier *id = name->asNameId();
                if (!id)
                    break;
                _namespaceNames += id;
            }
            _currentDepth = 0;

            accept(translationUnit()->ast());
        }

        if (lastToken == _bestToken.get()) // No matching namespace found
            translationUnit()->getTokenPosition(lastToken, line, column);
        else // Insert at end of matching namespace
            translationUnit()->getTokenEndPosition(_bestToken.get(), line, column);
    }

protected:
    bool preVisit(AST *ast) override
    {
        return ast->asNamespace() || ast->asTranslationUnit() || ast->asLinkageBody();
    }

    bool visit(NamespaceAST *ast) override
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
    int _line = 0;
    int _column = 0;
public:
    explicit FindFunctionDefinition(TranslationUnit *translationUnit)
        : ASTVisitor(translationUnit)
    {
    }

    FunctionDefinitionAST *operator()(int line, int column)
    {
        _result = nullptr;
        _line = line;
        _column = column;
        accept(translationUnit()->ast());
        return _result;
    }

protected:
    bool preVisit(AST *ast) override
    {
        if (_result)
            return false;
        int line, column;
        translationUnit()->getTokenPosition(ast->firstToken(), &line, &column);
        if (line > _line || (line == _line && column > _column))
            return false;
        translationUnit()->getTokenEndPosition(ast->lastToken() - 1, &line, &column);
        if (line < _line || (line == _line && column < _column))
            return false;
        return true;
    }

    bool visit(FunctionDefinitionAST *ast) override
    {
        _result = ast;
        return false;
    }
};

} // anonymous namespace

static Declaration *isNonVirtualFunctionDeclaration(Symbol *s)
{
    if (!s)
        return nullptr;
    Declaration *declaration = s->asDeclaration();
    if (!declaration)
        return nullptr;
    Function *type = s->type()->asFunctionType();
    if (!type || type->isPureVirtual())
        return nullptr;
    return declaration;
}

static InsertionLocation nextToSurroundingDefinitions(Symbol *declaration,
                                                      const CppRefactoringChanges &changes,
                                                      const FilePath &destinationFile)
{
    InsertionLocation noResult;
    Class *klass = declaration->enclosingClass();
    if (!klass)
        return noResult;

    // find the index of declaration
    int declIndex = -1;
    for (int i = 0; i < klass->memberCount(); ++i) {
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
    Function *definitionFunction = nullptr;
    QString prefix, suffix;
    Declaration *surroundingFunctionDecl = nullptr;
    for (int i = declIndex - 1; i >= 0; --i) {
        Symbol *s = klass->memberAt(i);
        if (s->isGenerated() || !(surroundingFunctionDecl = isNonVirtualFunctionDeclaration(s)))
            continue;
        if ((definitionFunction = symbolFinder.findMatchingDefinition(surroundingFunctionDecl,
                                                                      changes.snapshot())))
        {
            if (destinationFile.isEmpty() || destinationFile == definitionFunction->filePath()) {
                prefix = QLatin1String("\n\n");
                break;
            }
            definitionFunction = nullptr;
        }
    }
    if (!definitionFunction) {
        // try to find one below
        for (int i = declIndex + 1; i < klass->memberCount(); ++i) {
            Symbol *s = klass->memberAt(i);
            surroundingFunctionDecl = isNonVirtualFunctionDeclaration(s);
            if (!surroundingFunctionDecl)
                continue;
            if ((definitionFunction = symbolFinder.findMatchingDefinition(surroundingFunctionDecl,
                                                                          changes.snapshot())))
            {
                if (destinationFile.isEmpty() || destinationFile ==  definitionFunction->filePath()) {
                    suffix = QLatin1String("\n\n");
                    break;
                }
                definitionFunction = nullptr;
            }
        }
    }

    if (!definitionFunction)
        return noResult;

    int line, column;
    if (suffix.isEmpty()) {
        Document::Ptr targetDoc = changes.snapshot().document(definitionFunction->filePath());
        if (!targetDoc)
            return noResult;

        targetDoc->translationUnit()->getPosition(definitionFunction->endOffset(), &line, &column);
    } else {
        // we don't have an offset to the start of the function definition, so we need to manually find it...
        CppRefactoringFilePtr targetFile = changes.file(definitionFunction->filePath());
        if (!targetFile->isValid())
            return noResult;

        FindFunctionDefinition finder(targetFile->cppDocument()->translationUnit());
        FunctionDefinitionAST *functionDefinition = finder(definitionFunction->line(), definitionFunction->column());
        if (!functionDefinition)
            return noResult;

        targetFile->cppDocument()->translationUnit()->getTokenPosition(functionDefinition->firstToken(), &line, &column);
        const QList<AST *> path = ASTPath(targetFile->cppDocument())(line, column);
        for (auto it = path.rbegin(); it != path.rend(); ++it) {
            if (const auto templateDecl = (*it)->asTemplateDeclaration()) {
                if (templateDecl->declaration == functionDefinition) {
                    targetFile->cppDocument()->translationUnit()->getTokenPosition(
                                templateDecl->firstToken(), &line, &column);
                }
                break;
            }
        }
    }

    return InsertionLocation(definitionFunction->filePath(), prefix, suffix, line, column);
}

const QList<InsertionLocation> InsertionPointLocator::methodDefinition(
        Symbol *declaration, bool useSymbolFinder, const FilePath &destinationFile) const
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

    const FilePath declFilePath = declaration->filePath();
    FilePath target = declFilePath;
    if (!ProjectFile::isSource(ProjectFile::classify(declFilePath.path()))) {
        FilePath candidate = correspondingHeaderOrSource(declFilePath);
        if (!candidate.isEmpty())
            target = candidate;
    }

    CppRefactoringFilePtr targetFile = m_refactoringChanges.file(target);
    Document::Ptr doc = targetFile->cppDocument();
    if (doc.isNull())
        return result;

    int line = 0, column = 0;
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

/**
 * @brief getListOfMissingNamespacesForLocation checks which namespaces are present at a given
 * location and returns a list of namespace names that are needed to get the wanted namespace
 * @param file The file of the location
 * @param wantedNamespaces the namespace as list that should exists at the insert location
 * @param loc The location that should be checked (the namespaces should be available there)
 * @return A list of namespaces that are missing to reach the wanted namespaces.
 */
static QStringList getListOfMissingNamespacesForLocation(const CppRefactoringFile *file,
                                                  const QStringList &wantedNamespaces,
                                                  InsertionLocation loc)
{
    NSCheckerVisitor visitor(file, wantedNamespaces, file->position(loc.line(), loc.column()));
    visitor.accept(file->cppDocument()->translationUnit()->ast());
    return visitor.remainingNamespaces();
}

/**
 * @brief getNamespaceNames Returns a list of namespaces for an enclosing namespaces of a
 * namespace (contains the namespace itself)
 * @param firstNamespace the starting namespace (included in the list)
 * @return the enclosing namespaces, the outermost namespace is at the first index, the innermost
 * at the last index
 */
QStringList getNamespaceNames(const Namespace *firstNamespace)
{
    QStringList namespaces;
    for (const Namespace *scope = firstNamespace; scope; scope = scope->enclosingNamespace()) {
        if (scope->name() && scope->name()->identifier()) {
            namespaces.prepend(QString::fromUtf8(scope->name()->identifier()->chars(),
                                                 scope->name()->identifier()->size()));
        } else {
            namespaces.prepend(""); // an unnamed namespace
        }
    }
    namespaces.pop_front(); // the "global namespace" is one namespace, but not an unnamed
    return namespaces;
}

/**
 * @brief getNamespaceNames Returns a list of enclosing namespaces for a symbol
 * @param symbol a symbol from which we want the enclosing namespaces
 * @return the enclosing namespaces, the outermost namespace is at the first index, the innermost
 * at the last index
 */
QStringList getNamespaceNames(const Symbol *symbol)
{
    return getNamespaceNames(symbol->enclosingNamespace());
}

InsertionLocation insertLocationForMethodDefinition(Symbol *symbol,
                                                    const bool useSymbolFinder,
                                                    NamespaceHandling namespaceHandling,
                                                    const CppRefactoringChanges &refactoring,
                                                    const FilePath &filePath,
                                                    QStringList *insertedNamespaces)
{
    QTC_ASSERT(symbol, return InsertionLocation());

    CppRefactoringFilePtr file = refactoring.file(filePath);
    QStringList requiredNamespaces;
    if (namespaceHandling == NamespaceHandling::CreateMissing) {
        requiredNamespaces = getNamespaceNames(symbol);
    }

    // Try to find optimal location
    // FIXME: The locator should not return a valid location if the namespaces don't match
    //        (or provide enough context).
    const InsertionPointLocator locator(refactoring);
    const QList<InsertionLocation> list
            = locator.methodDefinition(symbol, useSymbolFinder, filePath);
    const bool isHeader = ProjectFile::isHeader(ProjectFile::classify(filePath.path()));
    const bool hasIncludeGuard = isHeader
            && !file->cppDocument()->includeGuardMacroName().isEmpty();
    int lastLine;
    if (hasIncludeGuard) {
        const TranslationUnit * const tu = file->cppDocument()->translationUnit();
        tu->getTokenPosition(tu->ast()->lastToken(), &lastLine);
    }
    int i = 0;
    for ( ; i < list.count(); ++i) {
        InsertionLocation location = list.at(i);
        if (!location.isValid() || location.filePath() != filePath)
            continue;
        if (hasIncludeGuard && location.line() == lastLine)
            continue;
        if (!requiredNamespaces.isEmpty()) {
            QStringList missing = getListOfMissingNamespacesForLocation(file.get(),
                                                                        requiredNamespaces,
                                                                        location);
            if (!missing.isEmpty())
                continue;
        }
        return location;
    }

    // ...failed,
    // if class member try to get position right after class
    int line = 0, column = 0;
    if (Class *clazz = symbol->enclosingClass()) {
        if (symbol->filePath() == filePath) {
            file->cppDocument()->translationUnit()->getPosition(clazz->endOffset(), &line, &column);
            if (line != 0) {
                ++column; // Skipping the ";"
                return InsertionLocation(filePath, QLatin1String("\n\n"), QLatin1String(""),
                                         line, column);
            }
        }
    }

    // fall through: position at end of file, unless we find a matching namespace
    const QTextDocument *doc = file->document();
    int pos = qMax(0, doc->characterCount() - 1);
    QString prefix = "\n\n";
    QString suffix = "\n\n";
    NSVisitor visitor(file.data(), requiredNamespaces, pos);
    visitor.accept(file->cppDocument()->translationUnit()->ast());
    if (visitor.enclosingNamespace())
        pos = file->startOf(visitor.enclosingNamespace()->linkage_body) + 1;
    for (const QString &ns : visitor.remainingNamespaces()) {
        prefix += "namespace " + ns + " {\n";
        suffix += "}\n";
    }
    if (insertedNamespaces)
        *insertedNamespaces = visitor.remainingNamespaces();

    //TODO watch for moc-includes

    file->lineAndColumn(pos, &line, &column);
    return InsertionLocation(filePath, prefix, suffix, line, column);
}

} // namespace CppEditor;
