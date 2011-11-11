/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "cppinsertdecldef.h"
#include "cppquickfixassistant.h"

#include <CPlusPlus.h>
#include <cplusplus/ASTPath.h>
#include <cplusplus/CppRewriter.h>
#include <cplusplus/LookupContext.h>
#include <cplusplus/Overview.h>
#include <cpptools/insertionpointlocator.h>
#include <cpptools/cpprefactoringchanges.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>

using namespace CPlusPlus;
using namespace CppEditor;
using namespace CppEditor::Internal;
using namespace CppTools;

namespace {

class InsertDeclOperation: public CppQuickFixOperation
{
public:
    InsertDeclOperation(const QSharedPointer<const Internal::CppQuickFixAssistInterface> &interface,
                        const QString &targetFileName, const Class *targetSymbol,
                        InsertionPointLocator::AccessSpec xsSpec,
                        const QString &decl)
        : CppQuickFixOperation(interface, 0)
        , m_targetFileName(targetFileName)
        , m_targetSymbol(targetSymbol)
        , m_xsSpec(xsSpec)
        , m_decl(decl)
    {
        QString type;
        switch (xsSpec) {
        case InsertionPointLocator::Public: type = QLatin1String("public"); break;
        case InsertionPointLocator::Protected: type = QLatin1String("protected"); break;
        case InsertionPointLocator::Private: type = QLatin1String("private"); break;
        case InsertionPointLocator::PublicSlot: type = QLatin1String("public slot"); break;
        case InsertionPointLocator::ProtectedSlot: type = QLatin1String("protected slot"); break;
        case InsertionPointLocator::PrivateSlot: type = QLatin1String("private slot"); break;
        default: break;
        }

        setDescription(QCoreApplication::translate("CppEditor::InsertDeclOperation",
                                                   "Add %1 Declaration").arg(type));
    }

    void performChanges(const CppRefactoringFilePtr &,
                        const CppRefactoringChanges &refactoring)
    {
        InsertionPointLocator locator(refactoring);
        const InsertionLocation loc = locator.methodDeclarationInClass(
                    m_targetFileName, m_targetSymbol, m_xsSpec);
        Q_ASSERT(loc.isValid());

        CppRefactoringFilePtr targetFile = refactoring.file(m_targetFileName);
        int targetPosition1 = targetFile->position(loc.line(), loc.column());
        int targetPosition2 = qMax(0, targetFile->position(loc.line(), 1) - 1);

        Utils::ChangeSet target;
        target.insert(targetPosition1, loc.prefix() + m_decl);
        targetFile->setChangeSet(target);
        targetFile->appendIndentRange(Utils::ChangeSet::Range(targetPosition2, targetPosition1));
        targetFile->setOpenEditor(true, targetPosition1);
        targetFile->apply();
    }

private:
    QString m_targetFileName;
    const Class *m_targetSymbol;
    InsertionPointLocator::AccessSpec m_xsSpec;
    QString m_decl;
};

} // anonymous namespace

QList<CppQuickFixOperation::Ptr> DeclFromDef::match(
    const QSharedPointer<const Internal::CppQuickFixAssistInterface> &interface)
{
    const QList<AST *> &path = interface->path();
    CppRefactoringFilePtr file = interface->currentFile();

    FunctionDefinitionAST *funDef = 0;
    int idx = 0;
    for (; idx < path.size(); ++idx) {
        AST *node = path.at(idx);
        if (idx > 1) {
            if (DeclaratorIdAST *declId = node->asDeclaratorId()) {
                if (file->isCursorOn(declId)) {
                    if (FunctionDefinitionAST *candidate = path.at(idx - 2)->asFunctionDefinition()) {
                        if (funDef) {
                            return noResult();
                        } else {
                            funDef = candidate;
                            break;
                        }
                    }
                }
            }
        }

        if (node->asClassSpecifier()) {
            return noResult();
        }
    }

    if (!funDef || !funDef->symbol)
        return noResult();

    Function *method = funDef->symbol;

    Scope *enclosingScope = method->enclosingScope();
    while (! (enclosingScope->isNamespace() || enclosingScope->isClass()))
        enclosingScope = enclosingScope->enclosingScope();
    Q_ASSERT(enclosingScope != 0);

    const Name *functionName = method->name();
    if (! functionName)
        return noResult(); // warn, anonymous function names are not valid c++

    if (! functionName->isQualifiedNameId())
        return noResult(); // warn, trying to add a declaration for a global function

    const QualifiedNameId *q = functionName->asQualifiedNameId();
    if (!q->base())
        return noResult();

    if (ClassOrNamespace *binding = interface->context().lookupType(q->base(), enclosingScope)) {
        foreach (Symbol *s, binding->symbols()) {
            if (Class *matchingClass = s->asClass()) {
                for (Symbol *s = matchingClass->find(q->identifier()); s; s = s->next()) {
                    if (! s->name())
                        continue;
                    else if (! q->identifier()->isEqualTo(s->identifier()))
                        continue;
                    else if (! s->type()->isFunctionType())
                        continue;

                    if (s->type().isEqualTo(method->type()))
                        return noResult();
                }

                // a good candidate

                const QString fn = QString::fromUtf8(matchingClass->fileName(),
                                                     matchingClass->fileNameLength());
                const QString decl = generateDeclaration(interface,
                                                         method,
                                                         binding);
                return singleResult(
                            new InsertDeclOperation(interface, fn, matchingClass,
                                                    InsertionPointLocator::Public,
                                                    decl));
            }
        }
    }

    return noResult();
}

QString DeclFromDef::generateDeclaration(const QSharedPointer<const Internal::CppQuickFixAssistInterface> &,
                                         Function *method,
                                         ClassOrNamespace *targetBinding)
{
    Q_UNUSED(targetBinding);

    Overview oo;
    oo.setShowFunctionSignatures(true);
    oo.setShowReturnTypes(true);
    oo.setShowArgumentNames(true);

    QString decl;
    decl += oo(method->type(), method->unqualifiedName());
    decl += QLatin1String(";\n");

    return decl;
}

namespace {

class InsertDefOperation: public CppQuickFixOperation
{
public:
    InsertDefOperation(const QSharedPointer<const Internal::CppQuickFixAssistInterface> &interface,
                       Declaration *decl, const InsertionLocation &loc)
        : CppQuickFixOperation(interface, 0)
        , m_decl(decl)
        , m_loc(loc)
    {
        const QString declFile = QString::fromUtf8(decl->fileName(), decl->fileNameLength());
        const QDir dir = QFileInfo(declFile).dir();
        setDescription(QCoreApplication::translate("CppEditor::InsertDefOperation",
                                                   "Add Definition in %1")
                       .arg(dir.relativeFilePath(m_loc.fileName())));
    }

    void performChanges(const CppRefactoringFilePtr &,
                        const CppRefactoringChanges &refactoring)
    {
        Q_ASSERT(m_loc.isValid());

        CppRefactoringFilePtr targetFile = refactoring.file(m_loc.fileName());

        Overview oo;
        oo.setShowFunctionSignatures(true);
        oo.setShowReturnTypes(true);
        oo.setShowArgumentNames(true);

        // make target lookup context
        Document::Ptr targetDoc = targetFile->cppDocument();
        Scope *targetScope = targetDoc->scopeAt(m_loc.line(), m_loc.column());
        LookupContext targetContext(targetDoc, assistInterface()->snapshot());
        ClassOrNamespace *targetCoN = targetContext.lookupType(targetScope);
        if (!targetCoN)
            targetCoN = targetContext.globalNamespace();

        // setup rewriting to get minimally qualified names
        SubstitutionEnvironment env;
        env.setContext(assistInterface()->context());
        env.switchScope(m_decl->enclosingScope());
        UseMinimalNames q(targetCoN);
        env.enter(&q);
        Control *control = assistInterface()->context().control().data();

        // rewrite the function type
        FullySpecifiedType tn = rewriteType(m_decl->type(), &env, control);

        // rewrite the function name
        QString name = oo(LookupContext::minimalName(m_decl, targetCoN, control));

        QString defText = oo.prettyType(tn, name) + "\n{\n}";

        int targetPos = targetFile->position(m_loc.line(), m_loc.column());
        int targetPos2 = qMax(0, targetFile->position(m_loc.line(), 1) - 1);

        Utils::ChangeSet target;
        target.insert(targetPos,  m_loc.prefix() + defText + m_loc.suffix());
        targetFile->setChangeSet(target);
        targetFile->appendIndentRange(Utils::ChangeSet::Range(targetPos2, targetPos));
        targetFile->setOpenEditor(true, targetPos);
        targetFile->apply();
    }

private:
    Declaration *m_decl;
    InsertionLocation m_loc;
};

} // anonymous namespace

QList<CppQuickFixOperation::Ptr> DefFromDecl::match(
    const QSharedPointer<const Internal::CppQuickFixAssistInterface> &interface)
{
    const QList<AST *> &path = interface->path();

    int idx = path.size() - 1;
    for (; idx >= 0; --idx) {
        AST *node = path.at(idx);
        if (SimpleDeclarationAST *simpleDecl = node->asSimpleDeclaration()) {
            if (simpleDecl->symbols && ! simpleDecl->symbols->next) {
                if (Symbol *symbol = simpleDecl->symbols->value) {
                    if (Declaration *decl = symbol->asDeclaration()) {
                        if (decl->type()->isFunctionType()
                                && !decl->type()->asFunctionType()->isPureVirtual()
                                && decl->enclosingScope()
                                && decl->enclosingScope()->isClass()) {
                            CppRefactoringChanges refactoring(interface->snapshot());
                            InsertionPointLocator locator(refactoring);
                            QList<CppQuickFixOperation::Ptr> results;
                            foreach (const InsertionLocation &loc, locator.methodDefinition(decl)) {
                                if (loc.isValid())
                                    results.append(CppQuickFixOperation::Ptr(new InsertDefOperation(interface, decl, loc)));
                            }
                            return results;
                        }
                    }
                }
            }

            break;
        }
    }

    return noResult();
}
