#if defined(_MSC_VER)
#pragma warning(disable:4996)
#endif

#include "symbolfinder.h"

#include <Symbols.h>
#include <Names.h>
#include <Literals.h>
#include <SymbolVisitor.h>
#include <Control.h>
#include <LookupContext.h>

#include <QtCore/QDebug>

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
                    if (_oper->isEqualTo(name))
                        _result.append(fun);
            }
        } else if (const Identifier *id = _declaration->identifier()) {
            if (id->isEqualTo(fun->identifier()))
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


SymbolFinder::SymbolFinder(const QString &referenceFileName)
    : m_referenceFile(referenceFileName)
{}

SymbolFinder::SymbolFinder(const char *referenceFileName, unsigned referenceFileLength)
    : m_referenceFile(QString::fromUtf8(referenceFileName, referenceFileLength))
{}

// strict means the returned symbol has to match exactly,
// including argument count and argument types
Symbol *SymbolFinder::findMatchingDefinition(Symbol *declaration,
                                             const Snapshot &snapshot,
                                             bool strict)
{
    if (!declaration)
        return 0;

    QString declFile = QString::fromUtf8(declaration->fileName(), declaration->fileNameLength());
    Q_ASSERT(declFile == m_referenceFile);

    Document::Ptr thisDocument = snapshot.document(declFile);
    if (! thisDocument) {
        qWarning() << "undefined document:" << declaration->fileName();
        return 0;
    }

    Function *declarationTy = declaration->type()->asFunctionType();
    if (! declarationTy) {
        qWarning() << "not a function:" << declaration->fileName()
                   << declaration->line() << declaration->column();
        return 0;
    }

    foreach (const QString &fileName, fileIterationOrder(snapshot)) {
        Document::Ptr doc = snapshot.document(fileName);
        if (!doc) {
            clear(fileName);
            continue;
        }

        const Identifier *id = declaration->identifier();
        if (id && ! doc->control()->findIdentifier(id->chars(), id->size()))
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
        if (! result.isEmpty()) {
            LookupContext context(doc, snapshot);

            QList<Function *> viableFunctions;

            ClassOrNamespace *enclosingType = context.lookupType(declaration);
            if (! enclosingType)
                continue; // nothing to do

            foreach (Function *fun, result) {
                const QList<LookupItem> declarations = context.lookup(fun->name(), fun->enclosingScope());
                if (declarations.isEmpty())
                    continue;

                const LookupItem best = declarations.first();
                if (enclosingType == context.lookupType(best.declaration()))
                    viableFunctions.append(fun);
            }

            if (viableFunctions.isEmpty())
                continue;

            else if (! strict && viableFunctions.length() == 1)
                return viableFunctions.first();

            Function *best = 0;

            foreach (Function *fun, viableFunctions) {
                if (! (fun->unqualifiedName() && fun->unqualifiedName()->isEqualTo(declaration->unqualifiedName())))
                    continue;
                else if (fun->argumentCount() == declarationTy->argumentCount()) {
                    if (! strict && ! best)
                        best = fun;

                    unsigned argc = 0;
                    for (; argc < declarationTy->argumentCount(); ++argc) {
                        Symbol *arg = fun->argumentAt(argc);
                        Symbol *otherArg = declarationTy->argumentAt(argc);
                        if (! arg->type().isEqualTo(otherArg->type()))
                            break;
                    }

                    if (argc == declarationTy->argumentCount())
                        best = fun;
                }
            }

            if (strict && ! best)
                continue;

            if (! best)
                best = viableFunctions.first();
            return best;
        }
    }

    return 0;
}

Class *SymbolFinder::findMatchingClassDeclaration(Symbol *declaration, const Snapshot &snapshot)
{
    if (! declaration->identifier())
        return 0;

    QString declFile = QString::fromUtf8(declaration->fileName(), declaration->fileNameLength());
    Q_ASSERT(declFile == m_referenceFile);

    foreach (const QString &file, fileIterationOrder(snapshot)) {
        Document::Ptr doc = snapshot.document(file);
        if (!doc) {
            clear(file);
            continue;
        }

        if (! doc->control()->findIdentifier(declaration->identifier()->chars(),
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

void SymbolFinder::findMatchingDeclaration(const LookupContext &context,
                                           Function *functionType,
                                           QList<Declaration *> *typeMatch,
                                           QList<Declaration *> *argumentCountMatch,
                                           QList<Declaration *> *nameMatch)
{
    Scope *enclosingScope = functionType->enclosingScope();
    while (! (enclosingScope->isNamespace() || enclosingScope->isClass()))
        enclosingScope = enclosingScope->enclosingScope();
    Q_ASSERT(enclosingScope != 0);

    const Name *functionName = functionType->name();
    if (! functionName)
        return; // anonymous function names are not valid c++

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
    if (!funcId) // E.g. operator, which we might be able to handle in the future...
        return;

    foreach (Symbol *s, binding->symbols()) {
        Scope *scope = s->asScope();
        if (!scope)
            continue;

        for (Symbol *s = scope->find(funcId); s; s = s->next()) {
            if (! s->name())
                continue;
            else if (! funcId->isEqualTo(s->identifier()))
                continue;
            else if (! s->type()->isFunctionType())
                continue;
            else if (Declaration *decl = s->asDeclaration()) {
                if (Function *declFunTy = decl->type()->asFunctionType()) {
                    if (functionType->isEqualTo(declFunTy))
                        typeMatch->prepend(decl);
                    else if (functionType->argumentCount() == declFunTy->argumentCount())
                        argumentCountMatch->prepend(decl);
                    else
                        nameMatch->append(decl);
                }
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
    result.append(nameMatch);
    return result;
}

#include <QtCore/QThread>
QStringList SymbolFinder::fileIterationOrder(const Snapshot &snapshot)
{
    if (m_filePriorityCache.isEmpty()) {
        foreach (const Document::Ptr &doc, snapshot)
            insert(doc->fileName());
    } else {
        checkCacheConsistency(snapshot);
    }

    return m_filePriorityCache.values();
}

void SymbolFinder::checkCacheConsistency(const Snapshot &snapshot)
{
    // We only check for "new" files, which which are in the snapshot but not in the cache.
    // The counterpart validation for "old" files is done when one tries to access the
    // corresponding document and notices it's now null.
    foreach (const Document::Ptr &doc, snapshot) {
        if (!m_fileMetaCache.contains(doc->fileName()))
            insert(doc->fileName());
    }
}

void SymbolFinder::clear(const QString &comparingFile)
{
    m_filePriorityCache.remove(computeKey(m_referenceFile, comparingFile), comparingFile);
    m_fileMetaCache.remove(comparingFile);
}

void SymbolFinder::insert(const QString &comparingFile)
{
    // We want an ordering such that the documents with the most common path appear first.
    m_filePriorityCache.insert(computeKey(m_referenceFile, comparingFile), comparingFile);
    m_fileMetaCache.insert(comparingFile);
}

int SymbolFinder::computeKey(const QString &referenceFile, const QString &comparingFile)
{
    // As similar the path from the comparing file is to the path from the reference file,
    // the smaller the key is, which is then used for sorting the map.
    std::pair<QString::const_iterator,
              QString::const_iterator> r = std::mismatch(referenceFile.begin(),
                                                         referenceFile.end(),
                                                         comparingFile.begin());
    return referenceFile.length() - (r.first - referenceFile.begin());
}
