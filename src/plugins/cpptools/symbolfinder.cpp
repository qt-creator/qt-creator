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

#if defined(_MSC_VER)
#pragma warning(disable:4996)
#endif

#include "symbolfinder.h"

#include "cppmodelmanager.h"

#include <cplusplus/LookupContext.h>

#include <utils/qtcassert.h>

#include <QDebug>
#include <QPair>

#include <algorithm>
#include <utility>

using namespace CPlusPlus;
using namespace CppTools;

namespace {

struct Hit {
    Hit(Function *func, bool exact) : func(func), exact(exact) {}
    Hit() = default;

    Function *func = nullptr;
    bool exact = false;
};

class FindMatchingDefinition: public SymbolVisitor
{
    Symbol *_declaration = nullptr;
    const OperatorNameId *_oper = nullptr;
    const ConversionNameId *_conv = nullptr;
    const bool _strict;
    QList<Hit> _result;

public:
    explicit FindMatchingDefinition(Symbol *declaration, bool strict)
        : _declaration(declaration), _strict(strict)
    {
        if (_declaration->name()) {
            _oper = _declaration->name()->asOperatorNameId();
            _conv = _declaration->name()->asConversionNameId();
        }
    }

    const QList<Hit> result() const { return _result; }

    using SymbolVisitor::visit;

    bool visit(Function *fun) override
    {
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

} // end of anonymous namespace

static const int kMaxCacheSize = 10;

SymbolFinder::SymbolFinder() = default;

// strict means the returned symbol has to match exactly,
// including argument count, argument types, constness and volatileness.
Function *SymbolFinder::findMatchingDefinition(Symbol *declaration,
                                             const Snapshot &snapshot,
                                             bool strict)
{
    if (!declaration)
        return nullptr;

    QString declFile = QString::fromUtf8(declaration->fileName(), declaration->fileNameLength());

    Document::Ptr thisDocument = snapshot.document(declFile);
    if (!thisDocument) {
        qWarning() << "undefined document:" << declaration->fileName();
        return nullptr;
    }

    Function *declarationTy = declaration->type()->asFunctionType();
    if (!declarationTy) {
        qWarning() << "not a function:" << declaration->fileName()
                   << declaration->line() << declaration->column();
        return nullptr;
    }

    Hit best;
    foreach (const QString &fileName, fileIterationOrder(declFile, snapshot)) {
        Document::Ptr doc = snapshot.document(fileName);
        if (!doc) {
            clearCache(declFile, fileName);
            continue;
        }

        const Identifier *id = declaration->identifier();
        if (id && !doc->control()->findIdentifier(id->chars(), id->size()))
            continue;

        if (!id) {
            const Name * const name = declaration->name();
            if (!name)
                continue;
            if (const OperatorNameId * const oper = name->asOperatorNameId()) {
                if (!doc->control()->findOperatorNameId(oper->kind()))
                    continue;
            } else if (const ConversionNameId * const conv = name->asConversionNameId()) {
                if (!doc->control()->findConversionNameId(conv->type()))
                    continue;
            } else {
                continue;
            }
        }

        FindMatchingDefinition candidates(declaration, strict);
        candidates.accept(doc->globalNamespace());

        const QList<Hit> result = candidates.result();
        if (result.isEmpty())
            continue;

        LookupContext context(doc, snapshot);
        ClassOrNamespace *enclosingType = context.lookupType(declaration);
        if (!enclosingType)
            continue; // nothing to do

        for (const Hit &hit : result) {
            QTC_CHECK(!strict || hit.exact);

            const QList<LookupItem> declarations = context.lookup(hit.func->name(),
                                                                  hit.func->enclosingScope());
            if (declarations.isEmpty())
                continue;
            if (enclosingType != context.lookupType(declarations.first().declaration()))
                continue;

            if (hit.exact)
                return hit.func;

            if (!best.func || hit.func->argumentCount() == declarationTy->argumentCount())
                best = hit;
        }
    }

    QTC_CHECK(!best.exact);
    return strict ? nullptr : best.func;
}

Symbol *SymbolFinder::findMatchingVarDefinition(Symbol *declaration, const Snapshot &snapshot)
{
    if (!declaration)
        return nullptr;
    for (const Scope *s = declaration->enclosingScope(); s; s = s->enclosingScope()) {
        if (s->asBlock())
            return nullptr;
    }

    QString declFile = QString::fromUtf8(declaration->fileName(), declaration->fileNameLength());
    const Document::Ptr thisDocument = snapshot.document(declFile);
    if (!thisDocument) {
        qWarning() << "undefined document:" << declaration->fileName();
        return nullptr;
    }

    using SymbolWithPriority = QPair<Symbol *, bool>;
    QList<SymbolWithPriority> candidates;
    QList<SymbolWithPriority> fallbacks;
    foreach (const QString &fileName, fileIterationOrder(declFile, snapshot)) {
        Document::Ptr doc = snapshot.document(fileName);
        if (!doc) {
            clearCache(declFile, fileName);
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
                candidates << qMakePair(item.declaration(),
                                        context.lookupType(item.declaration()) == enclosingType);
            }
            // TODO: This is a workaround for static member definitions not being found by
            //       the lookup() function.
            if (addFallback)
                fallbacks << qMakePair(symbol, context.lookupType(symbol) == enclosingType);
        }
    }

    candidates << fallbacks;
    SymbolWithPriority best;
    for (const auto &candidate : qAsConst(candidates)) {
        if (candidate.first == declaration)
            continue;
        if (QLatin1String(candidate.first->fileName()) == declFile
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

    QString declFile = QString::fromUtf8(declaration->fileName(), declaration->fileNameLength());

    foreach (const QString &file, fileIterationOrder(declFile, snapshot)) {
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

        foreach (Symbol *s, type->symbols()) {
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
    while (!(enclosingScope->isNamespace() || enclosingScope->isClass()))
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

    foreach (Symbol *s, binding->symbols()) {
        Scope *scope = s->asScope();
        if (!scope)
            continue;

        if (funcId) {
            for (Symbol *s = scope->find(funcId); s; s = s->next()) {
                if (!s->name() || !funcId->match(s->identifier()) || !s->type()->isFunctionType())
                    continue;
                findDeclarationOfSymbol(s, functionType, typeMatch, argumentCountMatch, nameMatch);
            }
        } else {
            for (Symbol *s = scope->find(operatorNameId); s; s = s->next()) {
                if (!s->name() || !s->type()->isFunctionType())
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
    QList<Declaration *> fuzzyMatches = argumentCountMatch + nameMatch;
    if (!functionType->enclosingScope() || !functionType->enclosingScope()->isClass()) {
        for (Declaration * const d : fuzzyMatches) {
            if (d->enclosingScope() && d->enclosingScope()->isClass())
                result.append(d);
        }
    }
    return result;
}

QStringList SymbolFinder::fileIterationOrder(const QString &referenceFile, const Snapshot &snapshot)
{
    if (m_filePriorityCache.contains(referenceFile)) {
        checkCacheConsistency(referenceFile, snapshot);
    } else {
        foreach (Document::Ptr doc, snapshot)
            insertCache(referenceFile, doc->fileName());
    }

    QStringList files = m_filePriorityCache.value(referenceFile).toStringList();

    trackCacheUse(referenceFile);

    return files;
}

void SymbolFinder::clearCache()
{
    m_filePriorityCache.clear();
    m_fileMetaCache.clear();
    m_recent.clear();
}

void SymbolFinder::checkCacheConsistency(const QString &referenceFile, const Snapshot &snapshot)
{
    // We only check for "new" files, which which are in the snapshot but not in the cache.
    // The counterpart validation for "old" files is done when one tries to access the
    // corresponding document and notices it's now null.
    const QSet<QString> &meta = m_fileMetaCache.value(referenceFile);
    foreach (const Document::Ptr &doc, snapshot) {
        if (!meta.contains(doc->fileName()))
            insertCache(referenceFile, doc->fileName());
    }
}

const QString projectPartIdForFile(const QString &filePath)
{
    const QList<ProjectPart::ConstPtr> parts = CppModelManager::instance()->projectPart(filePath);
    if (!parts.isEmpty())
        return parts.first()->id();
    return QString();
}

void SymbolFinder::clearCache(const QString &referenceFile, const QString &comparingFile)
{
    m_filePriorityCache[referenceFile].remove(comparingFile, projectPartIdForFile(comparingFile));
    m_fileMetaCache[referenceFile].remove(comparingFile);
}

void SymbolFinder::insertCache(const QString &referenceFile, const QString &comparingFile)
{
    FileIterationOrder &order = m_filePriorityCache[referenceFile];
    if (!order.isValid()) {
        const auto projectPartId = projectPartIdForFile(referenceFile);
        order.setReference(referenceFile, projectPartId);
    }
    order.insert(comparingFile, projectPartIdForFile(comparingFile));

    m_fileMetaCache[referenceFile].insert(comparingFile);
}

void SymbolFinder::trackCacheUse(const QString &referenceFile)
{
    if (!m_recent.isEmpty()) {
        if (m_recent.last() == referenceFile)
            return;
        m_recent.removeOne(referenceFile);
    }

    m_recent.append(referenceFile);

    // We don't want this to grow too much.
    if (m_recent.size() > kMaxCacheSize) {
        const QString &oldest = m_recent.takeFirst();
        m_filePriorityCache.remove(oldest);
        m_fileMetaCache.remove(oldest);
    }
}
