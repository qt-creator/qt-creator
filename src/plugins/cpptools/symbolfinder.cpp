/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

#include <algorithm>
#include <utility>

using namespace CPlusPlus;
using namespace CppTools;

namespace {

class FindMatchingDefinition: public SymbolVisitor
{
    Symbol *_declaration;
    const OperatorNameId *_oper;
    QList<Function *> _result;

public:
    FindMatchingDefinition(Symbol *declaration)
        : _declaration(declaration)
        , _oper(0)
    {
        if (_declaration->name())
            _oper = _declaration->name()->asOperatorNameId();
    }

    QList<Function *> result() const { return _result; }

    using SymbolVisitor::visit;

    virtual bool visit(Function *fun)
    {
        if (_oper) {
            if (const Name *name = fun->unqualifiedName()) {
                    if (_oper->match(name))
                        _result.append(fun);
            }
        } else if (Function *decl = _declaration->type()->asFunctionType()) {
            if (fun->match(decl))
                _result.append(fun);
        }

        return false;
    }

    virtual bool visit(Block *)
    {
        return false;
    }
};

} // end of anonymous namespace

static const int kMaxCacheSize = 10;

SymbolFinder::SymbolFinder()
{}

// strict means the returned symbol has to match exactly,
// including argument count, argument types, constness and volatileness.
Function *SymbolFinder::findMatchingDefinition(Symbol *declaration,
                                             const Snapshot &snapshot,
                                             bool strict)
{
    if (!declaration)
        return 0;

    QString declFile = QString::fromUtf8(declaration->fileName(), declaration->fileNameLength());

    Document::Ptr thisDocument = snapshot.document(declFile);
    if (!thisDocument) {
        qWarning() << "undefined document:" << declaration->fileName();
        return 0;
    }

    Function *declarationTy = declaration->type()->asFunctionType();
    if (!declarationTy) {
        qWarning() << "not a function:" << declaration->fileName()
                   << declaration->line() << declaration->column();
        return 0;
    }

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
            if (!declaration->name())
                continue;
            const OperatorNameId *oper = declaration->name()->asOperatorNameId();
            if (!oper)
                continue;
            if (!doc->control()->findOperatorNameId(oper->kind()))
                continue;
        }

        FindMatchingDefinition candidates(declaration);
        candidates.accept(doc->globalNamespace());

        const QList<Function *> result = candidates.result();
        if (!result.isEmpty()) {
            LookupContext context(doc, snapshot);

            QList<Function *> viableFunctions;

            ClassOrNamespace *enclosingType = context.lookupType(declaration);
            if (!enclosingType)
                continue; // nothing to do

            foreach (Function *fun, result) {
                if (fun->unqualifiedName()->isDestructorNameId() != declaration->unqualifiedName()->isDestructorNameId())
                    continue;

                const QList<LookupItem> declarations = context.lookup(fun->name(), fun->enclosingScope());
                if (declarations.isEmpty())
                    continue;

                const LookupItem best = declarations.first();
                if (enclosingType == context.lookupType(best.declaration()))
                    viableFunctions.append(fun);
            }

            if (viableFunctions.isEmpty())
                continue;

            else if (!strict && viableFunctions.length() == 1)
                return viableFunctions.first();

            Function *best = 0;

            foreach (Function *fun, viableFunctions) {
                if (!(fun->unqualifiedName()
                      && fun->unqualifiedName()->match(declaration->unqualifiedName()))) {
                    continue;
                }
                if (fun->argumentCount() == declarationTy->argumentCount()) {
                    if (!strict && !best)
                        best = fun;

                    const unsigned argc = declarationTy->argumentCount();
                    unsigned argIt = 0;
                    for (; argIt < argc; ++argIt) {
                        Symbol *arg = fun->argumentAt(argIt);
                        Symbol *otherArg = declarationTy->argumentAt(argIt);
                        if (!arg->type().match(otherArg->type()))
                            break;
                    }

                    if (argIt == argc
                            && fun->isConst() == declaration->type().isConst()
                            && fun->isVolatile() == declaration->type().isVolatile()) {
                        best = fun;
                    }
                }
            }

            if (strict && !best)
                continue;

            if (!best)
                best = viableFunctions.first();
            return best;
        }
    }

    return 0;
}

Class *SymbolFinder::findMatchingClassDeclaration(Symbol *declaration, const Snapshot &snapshot)
{
    if (!declaration->identifier())
        return 0;

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

    return 0;
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
    QTC_ASSERT(enclosingScope != 0, return);

    const Name *functionName = functionType->name();
    if (!functionName)
        return;

    ClassOrNamespace *binding = 0;
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
    QList<Declaration *> nameMatch, argumentCountMatch, typeMatch;
    findMatchingDeclaration(context, functionType, &typeMatch, &argumentCountMatch, &nameMatch);
    result.append(typeMatch);
    result.append(argumentCountMatch);
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
    const QList<ProjectPart::Ptr> parts = CppModelManager::instance()->projectPart(filePath);
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
