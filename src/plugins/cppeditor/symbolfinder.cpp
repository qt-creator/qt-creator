// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#if defined(_MSC_VER)
#pragma warning(disable:4996)
#endif

#include "symbolfinder.h"

#include "cpplocatordata.h"
#include "cppmodelmanager.h"

#include <cplusplus/LookupContext.h>
#include <cplusplus/Overview.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QPair>

#include <utility>

using namespace CPlusPlus;
using namespace Utils;

namespace CppEditor {
namespace {

struct Hit {
    Hit(Function *func, bool exact) : func(func), exact(exact) {}
    Hit() = default;

    Function *func = nullptr;
    bool exact = false;
};

// The name of the scope a function is defined in: the innermost qualifier of an
// out-of-line definition, or the surrounding class of an in-class one. Telling a
// class from a namespace would require the binding, which is precisely what the
// caller wants to avoid computing, so a namespace name can be returned here.
const Identifier *definingScopeIdentifier(Symbol *symbol)
{
    if (const Name * const name = symbol->name()) {
        if (const QualifiedNameId * const qualified = name->asQualifiedNameId()) {
            const Name *qualifier = qualified->base();
            if (!qualifier)
                return nullptr; // Global scope, e.g. "::f".
            if (const QualifiedNameId * const nested = qualifier->asQualifiedNameId())
                qualifier = nested->name();
            return qualifier ? qualifier->identifier() : nullptr;
        }
    }
    if (const Class * const klass = symbol->enclosingClass())
        return klass->name() ? klass->name()->identifier() : nullptr;
    return nullptr;
}

class FindMatchingDefinition: public SymbolVisitor
{
    Symbol *_declaration = nullptr;
    const OperatorNameId *_oper = nullptr;
    const ConversionNameId *_conv = nullptr;
    const Identifier *_declaringClassId = nullptr;
    const bool _strict;
    QList<Hit> _result;

    // A function declared as a member of a class can only be defined as a member
    // of that same class, so a candidate whose defining scope has a different
    // name cannot be the definition we are after. Comparing the names is a
    // pointer comparison, while the exact check via LookupContext has to bind the
    // candidate document's entire include closure first, which dominates the cost
    // of searching a large snapshot.
    // Being a name comparison, this keeps a candidate defined in a namespace that
    // happens to be named like the class; the exact check then rejects it. It
    // also misses a definition that qualifies the class via a typedef, which is
    // legal but rare. Friends are declared inside a class without being members
    // of it, so they are exempt altogether.
    bool maybeDefinedInSameClass(Function *fun) const
    {
        if (!_declaringClassId)
            return true;
        const Identifier * const id = definingScopeIdentifier(fun);
        return id && id->equalTo(_declaringClassId);
    }

public:
    explicit FindMatchingDefinition(Symbol *declaration, bool strict)
        : _declaration(declaration), _strict(strict)
    {
        if (_declaration->name()) {
            _oper = _declaration->name()->asOperatorNameId();
            _conv = _declaration->name()->asConversionNameId();
        }
        if (!_declaration->isFriend()) {
            if (const Class * const klass = _declaration->enclosingClass()) {
                if (const Name * const name = klass->name())
                    _declaringClassId = name->identifier();
            }
        }
    }

    const QList<Hit> result() const { return _result; }

    using SymbolVisitor::visit;

    bool visit(Function *fun) override
    {
        if (!maybeDefinedInSameClass(fun))
            return false;

        if (_oper || _conv) {
            if (const Name *name = fun->unqualifiedName()) {
                if ((_oper && _oper->match(name)) || (_conv && _conv->match(name)))
                    _result.append({fun, true});
            }
        } else if (Function *decl = _declaration->type()->asFunctionType()) {
            if (fun->match(decl)) {
                _result.prepend({fun, true});
            } else if (!_strict
                       && Matcher::match(fun->unqualifiedName(), decl->unqualifiedName())) {
                _result.append({fun, false});
            }
        }

        return false;
    }

    bool visit(Block *) override
    {
        return false;
    }
};

class FindMatchingVarDefinition: public SymbolVisitor
{
    Symbol *_declaration = nullptr;
    QList<Declaration *> _result;
    const Identifier *_className = nullptr;

public:
    explicit FindMatchingVarDefinition(Symbol *declaration)
        : _declaration(declaration)
    {
        if (declaration->isStatic() && declaration->enclosingScope()->asClass()
                && declaration->enclosingClass()->asClass()->name()) {
            _className = declaration->enclosingScope()->name()->identifier();
        }
    }

    const QList<Declaration *> result() const { return _result; }

    using SymbolVisitor::visit;

    bool visit(Declaration *decl) override
    {
        if (!decl->type()->match(_declaration->type().type()))
            return false;
        if (!_declaration->identifier()->equalTo(decl->identifier()))
            return false;
        if (_className) {
            const QualifiedNameId * const qualName = decl->name()->asQualifiedNameId();
            if (!qualName)
                return false;
            if (!qualName->base() || !qualName->base()->identifier()->equalTo(_className))
                return false;
        }
        _result.append(decl);
        return false;
    }

    bool visit(Block *) override { return false; }
};

// The definitions of one declaration, collected document by document.
class MatchingDefinitions
{
public:
    MatchingDefinitions(Symbol *declaration, Function *declarationType, bool strict,
                        bool stopAtFirstResult)
        : m_declaration(declaration)
        , m_declarationType(declarationType)
        , m_strict(strict)
        , m_stopAtFirstResult(stopAtFirstResult)
    {}

    // Returns whether the search is done.
    bool addFrom(const Document::Ptr &doc, const Snapshot &snapshot)
    {
        const Identifier *id = m_declaration->identifier();
        if (id && !doc->control()->findIdentifier(id->chars(), id->size()))
            return false;

        if (!id) {
            const Name * const name = m_declaration->name();
            if (!name)
                return false;
            if (const OperatorNameId * const oper = name->asOperatorNameId()) {
                if (!doc->control()->findOperatorNameId(oper->kind()))
                    return false;
            } else if (const ConversionNameId * const conv = name->asConversionNameId()) {
                if (!doc->control()->findConversionNameId(conv->type()))
                    return false;
            } else {
                return false;
            }
        }

        FindMatchingDefinition candidates(m_declaration, m_strict);
        candidates.accept(doc->globalNamespace());

        const QList<Hit> hits = candidates.result();
        if (hits.isEmpty())
            return false;

        LookupContext context(doc, snapshot);
        ClassOrNamespace *enclosingType = context.lookupType(m_declaration);
        if (!enclosingType)
            return false; // nothing to do

        for (const Hit &hit : hits) {
            QTC_CHECK(!m_strict || hit.exact);

            const QList<LookupItem> declarations = context.lookup(hit.func->name(),
                                                                  hit.func->enclosingScope());
            if (declarations.isEmpty())
                continue;
            if (enclosingType != context.lookupType(declarations.first().declaration()))
                continue;

            if (hit.exact) {
                if (!m_bestAreExact)
                    m_best.clear();
                m_best << hit.func;
                if (m_stopAtFirstResult) {
                    m_done = true;
                    return true;
                }
                m_bestAreExact = true;
            }

            if (!m_bestAreExact
                || hit.func->argumentCount() == m_declarationType->argumentCount()) {
                m_best << hit.func;
            }
        }
        return false;
    }

    QList<Function *> result() const
    {
        if (m_done)
            return m_best;
        if (m_strict && !m_bestAreExact)
            return {};
        return m_best;
    }

private:
    Symbol * const m_declaration;
    Function * const m_declarationType;
    const bool m_strict;
    const bool m_stopAtFirstResult;
    QList<Function *> m_best;
    bool m_bestAreExact = false;
    bool m_done = false;
};

} // end of anonymous namespace

static const int kMaxCacheSize = 10;

SymbolFinder::SymbolFinder() = default;

// strict means the returned symbol has to match exactly,
// including argument count, argument types, constness and volatileness.
// The documents read on request are added to the snapshot, together with what they
// include, so that the returned symbols belong to it.
Function *SymbolFinder::findMatchingDefinition(Symbol *declaration,
                                             Snapshot &snapshot,
                                             bool strict)
{
    const QList<Function *> defs = findMatchingDefinitions(declaration, snapshot, strict, true);
    return defs.isEmpty() ? nullptr : defs.first();
}

// The innermost component of a scope as the locator index spells it, without template
// arguments, e.g. "Foo" for "NS::Foo<Bar::Baz>". A placeholder such as
// "<anonymous namespace>" stays whole, so that it matches no name.
static QStringView innermostScopeName(QStringView scope)
{
    int depth = 0;
    for (qsizetype i = scope.size() - 1; i >= 0; --i) {
        if (scope.at(i) == '>') {
            ++depth;
        } else if (scope.at(i) == '<') {
            --depth;
        } else if (depth == 0 && scope.at(i) == ':') {
            scope = scope.mid(i + 1);
            break;
        }
    }
    const qsizetype templateArgs = scope.indexOf('<');
    return templateArgs <= 0 ? scope : scope.left(templateArgs);
}

static QString identifierName(const Name *name)
{
    const Identifier * const id = name ? name->identifier() : nullptr;
    return id ? QString::fromUtf8(id->chars(), id->size()) : QString();
}

// The names that the innermost scope of the declaration's definition can have in the
// locator index. An empty name stands for the global namespace, an empty list means
// that any name can do.
// As in FindMatchingDefinition, a member can only be defined in a scope named like its
// class. A function of a namespace can only be defined in that namespace or, for an
// inline namespace, also in the namespaces enclosing it. Friends are declared inside
// a class without being members of it, so they are exempt.
static QStringList definingScopeNames(Symbol *declaration)
{
    if (declaration->isFriend())
        return {};
    if (const Class * const klass = declaration->enclosingClass()) {
        const QString className = identifierName(klass->name());
        return className.isEmpty() ? QStringList() : QStringList(className);
    }
    QStringList names;
    for (const Namespace *ns = declaration->enclosingNamespace(); ns;
         ns = ns->enclosingNamespace()) {
        const bool isGlobal = !ns->enclosingNamespace();
        const QString name = identifierName(ns->name());
        if (!isGlobal && name.isEmpty())
            return {}; // Anonymous
        names.append(name);
        if (!ns->isInline())
            break;
    }
    return names;
}

static const QString projectPartIdForFile(const FilePath &filePath)
{
    const QList<ProjectPart::ConstPtr> parts = CppModelManager::projectPart(filePath);
    if (!parts.isEmpty())
        return parts.first()->id();
    return QString();
}

// The files outside of the snapshot that, according to the locator index, define a
// function of the declaration's name, those whose definition has the declaration's
// qualified name first, each in the order of closeness to the declaring file.
static FilePaths filesDefining(Symbol *declaration, const Snapshot &snapshot)
{
    CppLocatorData * const index = CppModelManager::locatorData();
    if (!index)
        return {};

    // As in SearchSymbols, which stores "Foo<T>" for members of templates and "~Foo"
    // for destructors.
    Overview overview;
    overview.showTemplateParameters = true;
    overview.includeWhiteSpaceInOperatorName = false;
    const QString name = overview.prettyName(declaration->unqualifiedName());
    if (name.isEmpty())
        return {};
    const QString qualified
        = overview.prettyName(LookupContext::fullyQualifiedName(declaration));

    const QStringList scopeNames = definingScopeNames(declaration);
    const auto maybeDefinedInSameScope = [&scopeNames](const IndexItem::Ptr &entry) {
        return scopeNames.isEmpty()
               || scopeNames.contains(innermostScopeName(entry->symbolScope()));
    };

    FilePaths qualifiedMatches;
    FilePaths plainMatches;
    const auto outsideSnapshot = [&snapshot](const FilePath &filePath) {
        return !snapshot.contains(filePath);
    };
    index->filterFiles(outsideSnapshot, [&](const IndexItem::Ptr &entry) {
        if ((entry->type() & IndexItem::Function) && entry->isFunctionDefinition()
            && entry->symbolName() == name) {
            if (!qualified.isEmpty() && entry->scopedSymbolName() == qualified) {
                if (!qualifiedMatches.contains(entry->filePath()))
                    qualifiedMatches.append(entry->filePath());
            } else if (maybeDefinedInSameScope(entry)) {
                if (!plainMatches.contains(entry->filePath()))
                    plainMatches.append(entry->filePath());
            }
        }
        if (entry->type() & IndexItem::Enum)
            return IndexItem::Continue;
        return IndexItem::Recurse;
    });

    const FilePath declFile = declaration->filePath();
    const auto byCloseness = [&declFile](FilePaths filePaths) {
        // Sorted first, as files of equal closeness keep their order.
        Utils::sort(filePaths);
        FileIterationOrder order(declFile, projectPartIdForFile(declFile));
        for (const FilePath &filePath : std::as_const(filePaths))
            order.insert(filePath, projectPartIdForFile(filePath));
        return order.toFilePaths();
    };
    FilePaths candidates = byCloseness(qualifiedMatches);
    for (const FilePath &filePath : byCloseness(plainMatches)) {
        if (!candidates.contains(filePath))
            candidates.append(filePath);
    }
    return candidates;
}

QList<Function *> SymbolFinder::findMatchingDefinitions(
    CPlusPlus::Symbol *declaration, CPlusPlus::Snapshot &snapshot, bool strict,
    bool stopAtFirstResult)
{
    if (!declaration)
        return {};

    const FilePath declFile = declaration->filePath();

    Document::Ptr thisDocument = snapshot.document(declFile);
    if (!thisDocument) {
        qWarning() << "undefined document:" << declaration->fileName();
        return {};
    }

    Function *declarationTy = declaration->type()->asFunctionType();
    if (!declarationTy) {
        qWarning() << "not a function:" << declaration->fileName()
        << declaration->line() << declaration->column();
        return {};
    }

    MatchingDefinitions matches(declaration, declarationTy, strict, stopAtFirstResult);

    // Returns whether the search is done.
    const auto scan = [&](const FilePaths &filePaths, bool mayRead) {
        for (const FilePath &filePath : filePaths) {
            Document::Ptr doc = snapshot.document(filePath);
            if (!doc && mayRead) {
                doc = CppModelManager::document(filePath, DocumentLookup::ReadIfNeeded);
                if (doc) {
                    const Snapshot globalSnapshot = CppModelManager::snapshot();
                    snapshot.insert(doc);
                    const QSet<FilePath> closure = globalSnapshot.allIncludesForDocument(filePath);
                    for (const FilePath &included : closure) {
                        if (snapshot.contains(included))
                            continue;
                        if (const Document::Ptr header = globalSnapshot.document(included))
                            snapshot.insert(header);
                    }
                }
            }
            if (!doc) {
                if (!mayRead) // The files to be read on request are not in the cache.
                    clearCache(declFile, filePath);
                continue;
            }
            if (matches.addFrom(doc, snapshot))
                return true;
        }
        return false;
    };

    // Unless the search already stopped at an exact definition, also search the files
    // outside of the snapshot: they can hold an exact definition that the snapshot lacks
    // or, when all definitions are asked for, more definitions.
    if (!scan(fileIterationOrder(declFile, snapshot), false))
        scan(filesDefining(declaration, snapshot), true);
    return matches.result();
}

Symbol *SymbolFinder::findMatchingVarDefinition(Symbol *declaration, const Snapshot &snapshot)
{
    if (!declaration)
        return nullptr;
    for (const Scope *s = declaration->enclosingScope(); s; s = s->enclosingScope()) {
        if (s->asBlock())
            return nullptr;
    }

    // Non-static data members cannot be defined outside of their class.
    const Scope * const scope = declaration->enclosingScope();
    if (scope && scope->asClass() && !declaration->type().isStatic())
        return nullptr;

    const FilePath declFile = declaration->filePath();
    const Document::Ptr thisDocument = snapshot.document(declFile);
    if (!thisDocument) {
        qWarning() << "undefined document:" << declaration->fileName();
        return nullptr;
    }

    using SymbolWithPriority = QPair<Symbol *, bool>;
    QList<SymbolWithPriority> candidates;
    QList<SymbolWithPriority> fallbacks;
    const FilePaths filePaths = fileIterationOrder(declFile, snapshot);
    for (const FilePath &filePath : filePaths) {
        Document::Ptr doc = snapshot.document(filePath);
        if (!doc) {
            clearCache(declFile, filePath);
            continue;
        }

        const Identifier *id = declaration->identifier();
        if (id && !doc->control()->findIdentifier(id->chars(), id->size()))
            continue;

        FindMatchingVarDefinition finder(declaration);
        finder.accept(doc->globalNamespace());
        if (finder.result().isEmpty())
            continue;

        LookupContext context(doc, snapshot);
        ClassOrNamespace * const enclosingType = context.lookupType(declaration);
        for (Symbol * const symbol : finder.result()) {
            const QList<LookupItem> items = context.lookup(symbol->name(),
                                                           symbol->enclosingScope());
            bool addFallback = true;
            for (const LookupItem &item : items) {
                if (item.declaration() == symbol)
                    addFallback = false;
                candidates.push_back({item.declaration(),
                                      context.lookupType(item.declaration()) == enclosingType});
            }
            // TODO: This is a workaround for static member definitions not being found by
            //       the lookup() function.
            if (addFallback)
                fallbacks.push_back({symbol, context.lookupType(symbol) == enclosingType});
        }
    }

    candidates << fallbacks;
    SymbolWithPriority best;
    for (const auto &candidate : std::as_const(candidates)) {
        if (candidate.first == declaration)
            continue;
        if (candidate.first->filePath() == declFile
                && candidate.first->sourceLocation() == declaration->sourceLocation())
            continue;
        if (!candidate.first->asDeclaration())
            continue;
        if (declaration->isExtern() && candidate.first->isStatic())
            continue;
        if (!best.first) {
            best = candidate;
            continue;
        }
        if (!best.second && candidate.second) {
            best = candidate;
            continue;
        }
        if (best.first->isExtern() && !candidate.first->isExtern())
            best = candidate;
    }

    return best.first;
}

Class *SymbolFinder::findMatchingClassDeclaration(Symbol *declaration, const Snapshot &snapshot)
{
    if (!declaration->identifier())
        return nullptr;

    const FilePath declFile = declaration->filePath();

    const FilePaths filePaths = fileIterationOrder(declFile, snapshot);
    for (const FilePath &file : filePaths) {
        Document::Ptr doc = snapshot.document(file);
        if (!doc) {
            clearCache(declFile, file);
            continue;
        }

        if (!doc->control()->findIdentifier(declaration->identifier()->chars(),
                                            declaration->identifier()->size()))
            continue;

        LookupContext context(doc, snapshot);

        ClassOrNamespace *type = context.lookupType(declaration);
        if (!type)
            continue;

        const QList<Symbol *> symbols = type->symbols();
        for (Symbol *s : symbols) {
            if (Class *c = s->asClass())
                return c;
        }
    }

    return nullptr;
}

static void findDeclarationOfSymbol(Symbol *s,
                                    Function *functionType,
                                    QList<Declaration *> *typeMatch,
                                    QList<Declaration *> *argumentCountMatch,
                                    QList<Declaration *> *nameMatch)
{
    if (Declaration *decl = s->asDeclaration()) {
        if (Function *declFunTy = decl->type()->asFunctionType()) {
            if (functionType->match(declFunTy))
                typeMatch->prepend(decl);
            else if (functionType->argumentCount() == declFunTy->argumentCount())
                argumentCountMatch->prepend(decl);
            else
                nameMatch->append(decl);
        }
    }
}

void SymbolFinder::findMatchingDeclaration(const LookupContext &context,
                                           Function *functionType,
                                           QList<Declaration *> *typeMatch,
                                           QList<Declaration *> *argumentCountMatch,
                                           QList<Declaration *> *nameMatch)
{
    if (!functionType)
        return;

    Scope *enclosingScope = functionType->enclosingScope();
    while (!(enclosingScope->asNamespace() || enclosingScope->asClass()))
        enclosingScope = enclosingScope->enclosingScope();
    QTC_ASSERT(enclosingScope != nullptr, return);

    const Name *functionName = functionType->name();
    if (!functionName)
        return;

    ClassOrNamespace *binding = nullptr;
    const QualifiedNameId *qName = functionName->asQualifiedNameId();
    if (qName) {
        if (qName->base())
            binding = context.lookupType(qName->base(), enclosingScope);
        else
            binding = context.globalNamespace();
        functionName = qName->name();
    }

    if (!binding) { // declaration for a global function
        binding = context.lookupType(enclosingScope);

        if (!binding)
            return;
    }

    const Identifier *funcId = functionName->identifier();
    OperatorNameId::Kind operatorNameId = OperatorNameId::InvalidOp;

    if (!funcId) {
        if (!qName)
            return;
        const OperatorNameId * const onid = qName->name()->asOperatorNameId();
        if (!onid)
            return;
        operatorNameId = onid->kind();
    }

    const QList<Symbol *> symbols = binding->symbols();
    for (Symbol *s : symbols) {
        Scope *scope = s->asScope();
        if (!scope)
            continue;

        if (funcId) {
            for (Symbol *s = scope->find(funcId); s; s = s->next()) {
                if (!s->name() || !funcId->match(s->identifier()) || !s->type()->asFunctionType())
                    continue;
                findDeclarationOfSymbol(s, functionType, typeMatch, argumentCountMatch, nameMatch);
            }
        } else {
            for (Symbol *s = scope->find(operatorNameId); s; s = s->next()) {
                const OperatorNameId *op = s->name() ? s->name()->asOperatorNameId() : nullptr;
                if (!op || op->kind() != operatorNameId || !s->type()->asFunctionType())
                    continue;
                findDeclarationOfSymbol(s, functionType, typeMatch, argumentCountMatch, nameMatch);
            }
        }
    }
}

QList<Declaration *> SymbolFinder::findMatchingDeclaration(const LookupContext &context,
                                                           Function *functionType)
{
    QList<Declaration *> result;
    if (!functionType)
        return result;

    QList<Declaration *> nameMatch, argumentCountMatch, typeMatch;
    findMatchingDeclaration(context, functionType, &typeMatch, &argumentCountMatch, &nameMatch);
    result.append(typeMatch);

    // For member functions not defined inline, add fuzzy matches as fallbacks. We cannot do
    // this for free functions, because there is no guarantee that there's a separate declaration.
    if (!functionType->enclosingScope() || !functionType->enclosingScope()->asClass()) {
        const QList<Declaration *> fuzzyMatches = argumentCountMatch + nameMatch;
        for (Declaration * const d : fuzzyMatches) {
            if (d->enclosingScope() && d->enclosingScope()->asClass())
                result.append(d);
        }
    }
    return result;
}

FilePaths SymbolFinder::fileIterationOrder(const FilePath &referenceFile, const Snapshot &snapshot)
{
    if (m_filePriorityCache.contains(referenceFile)) {
        checkCacheConsistency(referenceFile, snapshot);
    } else {
        for (Document::Ptr doc : snapshot)
            insertCache(referenceFile, doc->filePath());
    }

    FilePaths files = m_filePriorityCache.value(referenceFile).toFilePaths();

    trackCacheUse(referenceFile);

    return files;
}

void SymbolFinder::clearCache()
{
    m_filePriorityCache.clear();
    m_fileMetaCache.clear();
    m_recent.clear();
}

void SymbolFinder::checkCacheConsistency(const FilePath &referenceFile, const Snapshot &snapshot)
{
    // We only check for "new" files, which which are in the snapshot but not in the cache.
    // The counterpart validation for "old" files is done when one tries to access the
    // corresponding document and notices it's now null.
    const QSet<FilePath> &meta = m_fileMetaCache.value(referenceFile);
    for (const Document::Ptr &doc : snapshot) {
        if (!meta.contains(doc->filePath()))
            insertCache(referenceFile, doc->filePath());
    }
}

void SymbolFinder::clearCache(const FilePath &referenceFile, const FilePath &comparingFile)
{
    m_filePriorityCache[referenceFile].remove(comparingFile, projectPartIdForFile(comparingFile));
    m_fileMetaCache[referenceFile].remove(comparingFile);
}

void SymbolFinder::insertCache(const FilePath &referenceFile, const FilePath &comparingFile)
{
    FileIterationOrder &order = m_filePriorityCache[referenceFile];
    if (!order.isValid()) {
        const auto projectPartId = projectPartIdForFile(referenceFile);
        order.setReference(referenceFile, projectPartId);
    }
    order.insert(comparingFile, projectPartIdForFile(comparingFile));

    m_fileMetaCache[referenceFile].insert(comparingFile);
}

void SymbolFinder::trackCacheUse(const FilePath &referenceFile)
{
    if (!m_recent.isEmpty()) {
        if (m_recent.last() == referenceFile)
            return;
        m_recent.removeOne(referenceFile);
    }

    m_recent.append(referenceFile);

    // We don't want this to grow too much.
    if (m_recent.size() > kMaxCacheSize) {
        const FilePath &oldest = m_recent.takeFirst();
        m_filePriorityCache.remove(oldest);
        m_fileMetaCache.remove(oldest);
    }
}

} // namespace CppEditor
