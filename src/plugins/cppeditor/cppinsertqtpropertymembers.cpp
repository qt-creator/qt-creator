/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cppinsertqtpropertymembers.h"
#include "cppquickfixassistant.h"

#include <AST.h>
#include <Token.h>
#include <cplusplus/Overview.h>
#include <cplusplus/Symbols.h>
#include <cpptools/insertionpointlocator.h>
#include <cpptools/cpprefactoringchanges.h>
#include <cppeditor/cppquickfix.h>
#include <coreplugin/idocument.h>

#include <utils/qtcassert.h>

#include <QTextStream>

using namespace CPlusPlus;
using namespace CppTools;
using namespace TextEditor;
using namespace Utils;
using namespace CppEditor;
using namespace CppEditor::Internal;

void InsertQtPropertyMembers::match(const CppQuickFixInterface &interface,
    QuickFixOperations &result)
{
    const QList<AST *> &path = interface->path();

    if (path.isEmpty())
        return;

    AST * const ast = path.last();
    QtPropertyDeclarationAST *qtPropertyDeclaration = ast->asQtPropertyDeclaration();
    if (!qtPropertyDeclaration)
        return;

    ClassSpecifierAST *klass = 0;
    for (int i = path.size() - 2; i >= 0; --i) {
        klass = path.at(i)->asClassSpecifier();
        if (klass)
            break;
    }
    if (!klass)
        return;

    CppRefactoringFilePtr file = interface->currentFile();
    const QString propertyName = file->textOf(qtPropertyDeclaration->property_name);
    QString getterName;
    QString setterName;
    QString signalName;
    int generateFlags = 0;
    for (QtPropertyDeclarationItemListAST *it = qtPropertyDeclaration->property_declaration_item_list;
         it; it = it->next) {
        const char *tokenString = file->tokenAt(it->value->item_name_token).spell();
        if (!qstrcmp(tokenString, "READ")) {
            getterName = file->textOf(it->value->expression);
            generateFlags |= GenerateGetter;
        } else if (!qstrcmp(tokenString, "WRITE")) {
            setterName = file->textOf(it->value->expression);
            generateFlags |= GenerateSetter;
        } else if (!qstrcmp(tokenString, "NOTIFY")) {
            signalName = file->textOf(it->value->expression);
            generateFlags |= GenerateSignal;
        }
    }
    const QString storageName = QLatin1String("m_") +  propertyName;
    generateFlags |= GenerateStorage;

    Class *c = klass->symbol;

    Overview overview;
    for (unsigned i = 0; i < c->memberCount(); ++i) {
        Symbol *member = c->memberAt(i);
        FullySpecifiedType type = member->type();
        if (member->asFunction() || (type.isValid() && type->asFunctionType())) {
            const QString name = overview.prettyName(member->name());
            if (name == getterName)
                generateFlags &= ~GenerateGetter;
            else if (name == setterName)
                generateFlags &= ~GenerateSetter;
            else if (name == signalName)
                generateFlags &= ~GenerateSignal;
        } else if (member->asDeclaration()) {
            const QString name = overview.prettyName(member->name());
            if (name == storageName)
                generateFlags &= ~GenerateStorage;
        }
    }

    if (getterName.isEmpty() && setterName.isEmpty() && signalName.isEmpty())
        return;

    result.append(QuickFixOperation::Ptr(
        new Operation(interface, path.size() - 1, qtPropertyDeclaration, c,
            generateFlags, getterName, setterName, signalName, storageName)));
}

InsertQtPropertyMembers::Operation::Operation(
    const CppQuickFixInterface &interface,
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
    QString desc = InsertQtPropertyMembers::tr("Generate Missing Q_PROPERTY Members...");
    setDescription(desc);
}

void InsertQtPropertyMembers::Operation::perform()
{
    CppRefactoringChanges refactoring(snapshot());
    CppRefactoringFilePtr file = refactoring.file(fileName());

    InsertionPointLocator locator(refactoring);
    Utils::ChangeSet declarations;

    const QString typeName = file->textOf(m_declaration->type_id);
    const QString propertyName = file->textOf(m_declaration->property_name);

    // getter declaration
    if (m_generateFlags & GenerateGetter) {
        const QString getterDeclaration = typeName + QLatin1Char(' ') + m_getterName +
                QLatin1String("() const\n{\nreturn ") + m_storageName + QLatin1String(";\n}\n");
        InsertionLocation getterLoc = locator.methodDeclarationInClass(file->fileName(), m_class, InsertionPointLocator::Public);
        QTC_ASSERT(getterLoc.isValid(), return);
        insertAndIndent(file, &declarations, getterLoc, getterDeclaration);
    }

    // setter declaration
    if (m_generateFlags & GenerateSetter) {
        QString setterDeclaration;
        QTextStream setter(&setterDeclaration);
        setter << "void " << m_setterName << '(' << typeName << " arg)\n{\n";
        if (m_signalName.isEmpty()) {
            setter << m_storageName <<  " = arg;\n}\n";
        } else {
            setter << "if (" << m_storageName << " != arg) {\n" << m_storageName
                   << " = arg;\nemit " << m_signalName << "(arg);\n}\n}\n";
        }
        InsertionLocation setterLoc = locator.methodDeclarationInClass(file->fileName(), m_class, InsertionPointLocator::PublicSlot);
        QTC_ASSERT(setterLoc.isValid(), return);
        insertAndIndent(file, &declarations, setterLoc, setterDeclaration);
    }

    // signal declaration
    if (m_generateFlags & GenerateSignal) {
        const QString declaration = QLatin1String("void ") + m_signalName + QLatin1Char('(')
                                    + typeName + QLatin1String(" arg);\n");
        InsertionLocation loc = locator.methodDeclarationInClass(file->fileName(), m_class, InsertionPointLocator::Signals);
        QTC_ASSERT(loc.isValid(), return);
        insertAndIndent(file, &declarations, loc, declaration);
    }

    // storage
    if (m_generateFlags & GenerateStorage) {
        const QString storageDeclaration = typeName  + QLatin1String(" m_")
                                           + propertyName + QLatin1String(";\n");
        InsertionLocation storageLoc = locator.methodDeclarationInClass(file->fileName(), m_class, InsertionPointLocator::Private);
        QTC_ASSERT(storageLoc.isValid(), return);
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
