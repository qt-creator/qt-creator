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

#include "cppinsertqtpropertymembers.h"
#include "cppquickfixassistant.h"

#include <AST.h>
#include <Token.h>
#include <cplusplus/Overview.h>
#include <cplusplus/Symbols.h>
#include <cpptools/insertionpointlocator.h>
#include <cpptools/cpprefactoringchanges.h>
#include <cppeditor/cppquickfix.h>
#include <coreplugin/ifile.h>

using namespace CPlusPlus;
using namespace CppTools;
using namespace TextEditor;
using namespace Utils;
using namespace CppEditor;
using namespace CppEditor::Internal;

QList<CppQuickFixOperation::Ptr> InsertQtPropertyMembers::match(
    const QSharedPointer<const CppQuickFixAssistInterface> &interface)
{
    const QList<AST *> &path = interface->path();

    if (path.isEmpty())
        return noResult();

    AST * const ast = path.last();
    QtPropertyDeclarationAST *qtPropertyDeclaration = ast->asQtPropertyDeclaration();
    if (!qtPropertyDeclaration)
        return noResult();

    ClassSpecifierAST *klass = 0;
    for (int i = path.size() - 2; i >= 0; --i) {
        klass = path.at(i)->asClassSpecifier();
        if (klass)
            break;
    }
    if (!klass)
        return noResult();

    CppRefactoringFilePtr file = interface->currentFile();
    const QString propertyName = file->textOf(qtPropertyDeclaration->property_name);
    QString getterName;
    QString setterName;
    QString signalName;
    int generateFlags = 0;
    for (QtPropertyDeclarationItemListAST *it = qtPropertyDeclaration->property_declaration_item_list;
         it; it = it->next) {
        const QString tokenString = file->tokenAt(it->value->item_name_token).spell();
        if (tokenString == QLatin1String("READ")) {
            getterName = file->textOf(it->value->expression);
            generateFlags |= GenerateGetter;
        } else if (tokenString == QLatin1String("WRITE")) {
            setterName = file->textOf(it->value->expression);
            generateFlags |= GenerateSetter;
        } else if (tokenString == QLatin1String("NOTIFY")) {
            signalName = file->textOf(it->value->expression);
            generateFlags |= GenerateSignal;
        }
    }
    QString storageName = QString("m_%1").arg(propertyName);
    generateFlags |= GenerateStorage;

    Class *c = klass->symbol;

    Overview overview;
    for (unsigned i = 0; i < c->memberCount(); ++i) {
        Symbol *member = c->memberAt(i);
        FullySpecifiedType type = member->type();
        if (member->asFunction() || (type.isValid() && type->asFunctionType())) {
            const QString name = overview(member->name());
            if (name == getterName) {
                generateFlags &= ~GenerateGetter;
            } else if (name == setterName) {
                generateFlags &= ~GenerateSetter;
            } else if (name == signalName) {
                generateFlags &= ~GenerateSignal;
            }
        } else if (member->asDeclaration()) {
            const QString name = overview(member->name());
            if (name == storageName)
                generateFlags &= ~GenerateStorage;
        }
    }

    if (getterName.isEmpty() && setterName.isEmpty() && signalName.isEmpty())
        return noResult();

    return singleResult(new Operation(interface, path.size() - 1, qtPropertyDeclaration, c,
                                      generateFlags,
                                      getterName, setterName, signalName, storageName));
}

InsertQtPropertyMembers::Operation::Operation(
    const QSharedPointer<const CppQuickFixAssistInterface> &interface,
    int priority, QtPropertyDeclarationAST *declaration, Class *klass,
    int generateFlags, const QString &getterName, const QString &setterName, const QString &signalName,
    const QString &storageName)
    : CppQuickFixOperation(interface, priority)
    , m_declaration(declaration)
    , m_class(klass)
    , m_generateFlags(generateFlags)
    , m_getterName(getterName)
    , m_setterName(setterName)
    , m_signalName(signalName)
    , m_storageName(storageName)
{
    QString desc = InsertQtPropertyMembers::tr("Generate missing Q_PROPERTY members...");
    setDescription(desc);
}

void InsertQtPropertyMembers::Operation::performChanges(const CppRefactoringFilePtr &file,
                                                        const CppRefactoringChanges &refactoring)
{
    InsertionPointLocator locator(refactoring);
    Utils::ChangeSet declarations;

    const QString typeName = file->textOf(m_declaration->type_id);
    const QString propertyName = file->textOf(m_declaration->property_name);

    // getter declaration
    if (m_generateFlags & GenerateGetter) {
        //                const QString getterDeclaration = QString("%1 %2() const;").arg(typeName, getterName);
        const QString getterDeclaration = QString("%1 %2() const\n{\nreturn %3;\n}\n").arg(typeName, m_getterName, m_storageName);
        InsertionLocation getterLoc = locator.methodDeclarationInClass(file->fileName(), m_class, InsertionPointLocator::Public);
        Q_ASSERT(getterLoc.isValid());
        insertAndIndent(file, &declarations, getterLoc, getterDeclaration);
    }

    // setter declaration
    if (m_generateFlags & GenerateSetter) {
        //                const QString setterDeclaration = QString("void %1(%2 arg);").arg(setterName, typeName);
        QString setterDeclaration = QString("void %1(%2 arg)\n{\n").arg(m_setterName, typeName);
        if (!m_signalName.isEmpty()) {
            setterDeclaration += QString("if (%1 != arg) {\n%1 = arg;\nemit %2(arg);\n}\n}\n").arg(m_storageName, m_signalName);
        } else {
            setterDeclaration += QString("%1 = arg;\n}\n").arg(m_storageName);
        }
        InsertionLocation setterLoc = locator.methodDeclarationInClass(file->fileName(), m_class, InsertionPointLocator::PublicSlot);
        Q_ASSERT(setterLoc.isValid());
        insertAndIndent(file, &declarations, setterLoc, setterDeclaration);
    }

    // signal declaration
    if (m_generateFlags & GenerateSignal) {
        const QString declaration = QString("void %1(%2 arg);\n").arg(m_signalName, typeName);
        InsertionLocation loc = locator.methodDeclarationInClass(file->fileName(), m_class, InsertionPointLocator::Signals);
        Q_ASSERT(loc.isValid());
        insertAndIndent(file, &declarations, loc, declaration);
    }

    // storage
    if (m_generateFlags & GenerateStorage) {
        const QString storageDeclaration = QString("%1 m_%2;\n").arg(typeName, propertyName);
        InsertionLocation storageLoc = locator.methodDeclarationInClass(file->fileName(), m_class, InsertionPointLocator::Private);
        Q_ASSERT(storageLoc.isValid());
        insertAndIndent(file, &declarations, storageLoc, storageDeclaration);
    }

    file->setChangeSet(declarations);
    file->apply();
}

void InsertQtPropertyMembers::Operation::insertAndIndent(const RefactoringFilePtr &file, ChangeSet *changeSet, const InsertionLocation &loc, const QString &text)
{
    int targetPosition1 = file->position(loc.line(), loc.column());
    int targetPosition2 = qMax(0, file->position(loc.line(), 1) - 1);
    changeSet->insert(targetPosition1, loc.prefix() + text + loc.suffix());
    file->appendIndentRange(Utils::ChangeSet::Range(targetPosition2, targetPosition1));
}
