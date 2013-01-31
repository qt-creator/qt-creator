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

#include "cppelementevaluator.h"

#include <coreplugin/idocument.h>
#include <cpptools/cpptoolsreuse.h>

#include <FullySpecifiedType.h>
#include <Literals.h>
#include <Names.h>
#include <CoreTypes.h>
#include <Scope.h>
#include <Symbol.h>
#include <Symbols.h>
#include <cpptools/TypeHierarchyBuilder.h>
#include <cpptools/ModelManagerInterface.h>
#include <cplusplus/ExpressionUnderCursor.h>
#include <cplusplus/Overview.h>
#include <cplusplus/TypeOfExpression.h>
#include <cplusplus/LookupContext.h>
#include <cplusplus/LookupItem.h>
#include <cplusplus/Icons.h>

#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <QQueue>

using namespace CppEditor;
using namespace Internal;
using namespace CPlusPlus;

namespace {
    QStringList stripName(const QString &name) {
        QStringList all;
        all << name;
        int colonColon = 0;
        const int size = name.size();
        while ((colonColon = name.indexOf(QLatin1String("::"), colonColon)) != -1) {
            all << name.right(size - colonColon - 2);
            colonColon += 2;
        }
        return all;
    }
}

CppElementEvaluator::CppElementEvaluator(CPPEditorWidget *editor) :
    m_editor(editor),
    m_modelManager(CppModelManagerInterface::instance()),
    m_tc(editor->textCursor()),
    m_lookupBaseClasses(false),
    m_lookupDerivedClasses(false)
{}

void CppElementEvaluator::setTextCursor(const QTextCursor &tc)
{ m_tc = tc; }

void CppElementEvaluator::setLookupBaseClasses(const bool lookup)
{ m_lookupBaseClasses = lookup; }

void CppElementEvaluator::setLookupDerivedClasses(const bool lookup)
{ m_lookupDerivedClasses = lookup; }

// @todo: Consider refactoring code from CPPEditor::findLinkAt into here.
void CppElementEvaluator::execute()
{
    clear();

    if (!m_modelManager)
        return;

    const Snapshot &snapshot = m_modelManager->snapshot();
    Document::Ptr doc = snapshot.document(m_editor->editorDocument()->fileName());
    if (!doc)
        return;

    int line = 0;
    int column = 0;
    const int pos = m_tc.position();
    m_editor->convertPosition(pos, &line, &column);

    checkDiagnosticMessage(pos);

    if (!matchIncludeFile(doc, line) && !matchMacroInUse(doc, pos)) {
        CppTools::moveCursorToEndOfIdentifier(&m_tc);

        // Fetch the expression's code
        ExpressionUnderCursor expressionUnderCursor;
        const QString &expression = expressionUnderCursor(m_tc);
        Scope *scope = doc->scopeAt(line, column);

        TypeOfExpression typeOfExpression;
        typeOfExpression.init(doc, snapshot);
        // make possible to instantiate templates
        typeOfExpression.setExpandTemplates(true);
        const QList<LookupItem> &lookupItems = typeOfExpression(expression.toUtf8(), scope);
        if (lookupItems.isEmpty())
            return;

        const LookupItem &lookupItem = lookupItems.first(); // ### TODO: select best candidate.
        handleLookupItemMatch(snapshot, lookupItem, typeOfExpression.context());
    }
}

void CppElementEvaluator::checkDiagnosticMessage(int pos)
{
    foreach (const QTextEdit::ExtraSelection &sel,
             m_editor->extraSelections(TextEditor::BaseTextEditorWidget::CodeWarningsSelection)) {
        if (pos >= sel.cursor.selectionStart() && pos <= sel.cursor.selectionEnd()) {
            m_diagnosis = sel.format.toolTip();
            break;
        }
    }
}

bool CppElementEvaluator::matchIncludeFile(const CPlusPlus::Document::Ptr &document, unsigned line)
{
    foreach (const Document::Include &includeFile, document->includes()) {
        if (includeFile.line() == line) {
            m_element = QSharedPointer<CppElement>(new CppInclude(includeFile));
            return true;
        }
    }
    return false;
}

bool CppElementEvaluator::matchMacroInUse(const CPlusPlus::Document::Ptr &document, unsigned pos)
{
    foreach (const Document::MacroUse &use, document->macroUses()) {
        if (use.contains(pos)) {
            const unsigned begin = use.begin();
            if (pos < begin + use.macro().name().length()) {
                m_element = QSharedPointer<CppElement>(new CppMacro(use.macro()));
                return true;
            }
        }
    }
    return false;
}

void CppElementEvaluator::handleLookupItemMatch(const Snapshot &snapshot,
                                                const LookupItem &lookupItem,
                                                const LookupContext &context)
{
    Symbol *declaration = lookupItem.declaration();
    if (!declaration) {
        const QString &type = Overview().prettyType(lookupItem.type(), QString());
        m_element = QSharedPointer<CppElement>(new Unknown(type));
    } else {
        const FullySpecifiedType &type = declaration->type();
        if (declaration->isNamespace()) {
            m_element = QSharedPointer<CppElement>(new CppNamespace(declaration));
        } else if (declaration->isClass()
                   || declaration->isForwardClassDeclaration()
                   || (declaration->isTemplate() && declaration->asTemplate()->declaration()
                       && (declaration->asTemplate()->declaration()->isClass()
                           || declaration->asTemplate()->declaration()->isForwardClassDeclaration()))) {
            if (declaration->isForwardClassDeclaration())
                if (Symbol *classDeclaration =
                        m_symbolFinder.findMatchingClassDeclaration(declaration, snapshot)) {
                    declaration = classDeclaration;
                }
            CppClass *cppClass = new CppClass(declaration);
            if (m_lookupBaseClasses)
                cppClass->lookupBases(declaration, context);
            if (m_lookupDerivedClasses)
                cppClass->lookupDerived(declaration, snapshot);
            m_element = QSharedPointer<CppElement>(cppClass);
        } else if (Enum *enumDecl = declaration->asEnum()) {
            m_element = QSharedPointer<CppElement>(new CppEnum(enumDecl));
        } else if (EnumeratorDeclaration *enumerator = dynamic_cast<EnumeratorDeclaration *>(declaration)) {
            m_element = QSharedPointer<CppElement>(new CppEnumerator(enumerator));
        } else if (declaration->isTypedef()) {
            m_element = QSharedPointer<CppElement>(new CppTypedef(declaration));
        } else if (declaration->isFunction()
                   || (type.isValid() && type->isFunctionType())
                   || declaration->isTemplate()) {
            m_element = QSharedPointer<CppElement>(new CppFunction(declaration));
        } else if (declaration->isDeclaration() && type.isValid()) {
            m_element = QSharedPointer<CppElement>(
                new CppVariable(declaration, context, lookupItem.scope()));
        } else {
            m_element = QSharedPointer<CppElement>(new CppDeclarableElement(declaration));
        }
    }
}

bool CppElementEvaluator::identifiedCppElement() const
{
    return !m_element.isNull();
}

const QSharedPointer<CppElement> &CppElementEvaluator::cppElement() const
{
    return m_element;
}

bool CppElementEvaluator::hasDiagnosis() const
{
    return !m_diagnosis.isEmpty();
}

const QString &CppElementEvaluator::diagnosis() const
{
    return m_diagnosis;
}

void CppEditor::Internal::CppElementEvaluator::clear()
{
    m_element.clear();
    m_diagnosis.clear();
}

// CppElement
CppElement::CppElement() : helpCategory(TextEditor::HelpItem::Unknown)
{}

CppElement::~CppElement()
{}

// Unknown
Unknown::Unknown(const QString &type) : type(type)
{
    tooltip = type;
}


// CppInclude

CppInclude::CppInclude(const Document::Include &includeFile) :
    path(QDir::toNativeSeparators(includeFile.fileName())),
    fileName(QFileInfo(includeFile.fileName()).fileName())
{
    helpCategory = TextEditor::HelpItem::Brief;
    helpIdCandidates = QStringList(fileName);
    helpMark = fileName;
    link = CPPEditorWidget::Link(path);
    tooltip = path;
}

// CppMacro
CppMacro::CppMacro(const Macro &macro)
{
    helpCategory = TextEditor::HelpItem::Macro;
    const QString macroName = QLatin1String(macro.name());
    helpIdCandidates = QStringList(macroName);
    helpMark = macroName;
    link = CPPEditorWidget::Link(macro.fileName(), macro.line());
    tooltip = macro.toStringWithLineBreaks();
}

// CppDeclarableElement

CppDeclarableElement::CppDeclarableElement(Symbol *declaration) : CppElement()
{
    icon = Icons().iconForSymbol(declaration);

    Overview overview;
    overview.showArgumentNames = true;
    overview.showReturnTypes = true;
    name = overview.prettyName(declaration->name());
    if (declaration->enclosingScope()->isClass() ||
        declaration->enclosingScope()->isNamespace() ||
        declaration->enclosingScope()->isEnum()) {
        qualifiedName = overview.prettyName(LookupContext::fullyQualifiedName(declaration));
        helpIdCandidates = stripName(qualifiedName);
    } else {
        qualifiedName = name;
        helpIdCandidates.append(name);
    }

    tooltip = overview.prettyType(declaration->type(), qualifiedName);
    link = CPPEditorWidget::linkToSymbol(declaration);
    helpMark = name;
}

// CppNamespace
CppNamespace::CppNamespace(Symbol *declaration) : CppDeclarableElement(declaration)
{
    helpCategory = TextEditor::HelpItem::ClassOrNamespace;
    tooltip = qualifiedName;
}

// CppClass
CppClass::CppClass(Symbol *declaration) : CppDeclarableElement(declaration)
{
    helpCategory = TextEditor::HelpItem::ClassOrNamespace;
    tooltip = qualifiedName;
}

void CppClass::lookupBases(Symbol *declaration, const CPlusPlus::LookupContext &context)
{
    typedef QPair<ClassOrNamespace *, CppClass *> Data;

    if (ClassOrNamespace *clazz = context.lookupType(declaration)) {
        QSet<ClassOrNamespace *> visited;

        QQueue<Data> q;
        q.enqueue(qMakePair(clazz, this));
        while (!q.isEmpty()) {
            Data current = q.dequeue();
            clazz = current.first;
            visited.insert(clazz);
            const QList<ClassOrNamespace *> &bases = clazz->usings();
            foreach (ClassOrNamespace *baseClass, bases) {
                const QList<Symbol *> &symbols = baseClass->symbols();
                foreach (Symbol *symbol, symbols) {
                    if (symbol->isClass() && (
                        clazz = context.lookupType(symbol)) &&
                        !visited.contains(clazz)) {
                        CppClass baseCppClass(symbol);
                        CppClass *cppClass = current.second;
                        cppClass->bases.append(baseCppClass);
                        q.enqueue(qMakePair(clazz, &cppClass->bases.last()));
                    }
                }
            }
        }
    }
}

void CppClass::lookupDerived(CPlusPlus::Symbol *declaration, const CPlusPlus::Snapshot &snapshot)
{
    typedef QPair<CppClass *, TypeHierarchy> Data;

    TypeHierarchyBuilder builder(declaration, snapshot);
    const TypeHierarchy &completeHierarchy = builder.buildDerivedTypeHierarchy();

    QQueue<Data> q;
    q.enqueue(qMakePair(this, completeHierarchy));
    while (!q.isEmpty()) {
        const Data &current = q.dequeue();
        CppClass *clazz = current.first;
        const TypeHierarchy &classHierarchy = current.second;
        foreach (const TypeHierarchy &derivedHierarchy, classHierarchy.hierarchy()) {
            clazz->derived.append(CppClass(derivedHierarchy.symbol()));
            q.enqueue(qMakePair(&clazz->derived.last(), derivedHierarchy));
        }
    }
}

// CppFunction
CppFunction::CppFunction(Symbol *declaration)
    : CppDeclarableElement(declaration)
{
    helpCategory = TextEditor::HelpItem::Function;

    const FullySpecifiedType &type = declaration->type();

    // Functions marks can be found either by the main overload or signature based
    // (with no argument names and no return). Help ids have no signature at all.
    Overview overview;
    overview.showDefaultArguments = false;
    helpMark = overview.prettyType(type, name);

    overview.showFunctionSignatures = false;
    helpIdCandidates.append(overview.prettyName(declaration->name()));
}

// CppEnum
CppEnum::CppEnum(Enum *declaration)
    : CppDeclarableElement(declaration)
{
    helpCategory = TextEditor::HelpItem::Enum;
    tooltip = qualifiedName;
}

// CppTypedef
CppTypedef::CppTypedef(Symbol *declaration) : CppDeclarableElement(declaration)
{
    helpCategory = TextEditor::HelpItem::Typedef;
    tooltip = Overview().prettyType(declaration->type(), qualifiedName);
}

// CppVariable
CppVariable::CppVariable(Symbol *declaration, const LookupContext &context, Scope *scope) :
    CppDeclarableElement(declaration)
{
    const FullySpecifiedType &type = declaration->type();

    const Name *typeName = 0;
    if (type->isNamedType()) {
        typeName = type->asNamedType()->name();
    } else if (type->isPointerType() || type->isReferenceType()) {
        FullySpecifiedType associatedType;
        if (type->isPointerType())
            associatedType = type->asPointerType()->elementType();
        else
            associatedType = type->asReferenceType()->elementType();
        if (associatedType->isNamedType())
            typeName = associatedType->asNamedType()->name();
    }

    if (typeName) {
        if (ClassOrNamespace *clazz = context.lookupType(typeName, scope)) {
            if (!clazz->symbols().isEmpty()) {
                Overview overview;
                Symbol *symbol = clazz->symbols().at(0);
                const QString &name =
                    overview.prettyName(LookupContext::fullyQualifiedName(symbol));
                if (!name.isEmpty()) {
                    tooltip = name;
                    helpCategory = TextEditor::HelpItem::ClassOrNamespace;
                    const QStringList &allNames = stripName(name);
                    if (!allNames.isEmpty()) {
                        helpMark = allNames.last();
                        helpIdCandidates = allNames;
                    }
                }
            }
        }
    }
}

CppEnumerator::CppEnumerator(CPlusPlus::EnumeratorDeclaration *declaration)
    : CppDeclarableElement(declaration)
{
    helpCategory = TextEditor::HelpItem::Enum;

    Overview overview;

    Symbol *enumSymbol = declaration->enclosingScope()->asEnum();
    const QString enumName = overview.prettyName(LookupContext::fullyQualifiedName(enumSymbol));
    const QString enumeratorName = overview.prettyName(declaration->name());
    QString enumeratorValue;
    if (const StringLiteral *value = declaration->constantValue())
        enumeratorValue = QString::fromUtf8(value->chars(), value->size());

    helpMark = overview.prettyName(enumSymbol->name());

    tooltip = enumeratorName;
    if (!enumName.isEmpty())
        tooltip.prepend(enumName + QLatin1Char(' '));
    if (!enumeratorValue.isEmpty())
        tooltip.append(QLatin1String(" = ") + enumeratorValue);
}
