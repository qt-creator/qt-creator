/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "cppelementevaluator.h"

#include <coreplugin/ifile.h>
#include <cpptools/cppmodelmanagerinterface.h>

#include <FullySpecifiedType.h>
#include <Names.h>
#include <CoreTypes.h>
#include <Scope.h>
#include <Symbol.h>
#include <Symbols.h>
#include <cplusplus/ExpressionUnderCursor.h>
#include <cplusplus/Overview.h>
#include <cplusplus/TypeOfExpression.h>
#include <cplusplus/LookupContext.h>
#include <cplusplus/LookupItem.h>
#include <cplusplus/Icons.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QSet>
#include <QtCore/QQueue>

using namespace CppEditor;
using namespace Internal;
using namespace CPlusPlus;

namespace {
    void moveCursorToEndOfName(QTextCursor *tc) {
        QTextDocument *doc = tc->document();
        if (!doc)
            return;

        QChar ch = doc->characterAt(tc->position());
        while (ch.isLetterOrNumber() || ch == QLatin1Char('_')) {
            tc->movePosition(QTextCursor::NextCharacter);
            ch = doc->characterAt(tc->position());
        }
    }
}

CppElementEvaluator::CppElementEvaluator(CPPEditor *editor) :
    m_editor(editor),
    m_modelManager(CppTools::CppModelManagerInterface::instance()),
    m_tc(editor->textCursor()),
    m_lookupBaseClasses(false)
{}

void CppElementEvaluator::setTextCursor(const QTextCursor &tc)
{ m_tc = tc; }

void CppElementEvaluator::setLookupBaseClasses(const bool lookup)
{ m_lookupBaseClasses = lookup; }

QSharedPointer<CppElement> CppElementEvaluator::identifyCppElement()
{
    m_element.clear();
    evaluate();
    return m_element;
}

// @todo: Consider refactoring code from CPPEditor::findLinkAt into here.
void CppElementEvaluator::evaluate()
{
    if (!m_modelManager)
        return;

    const Snapshot &snapshot = m_modelManager->snapshot();
    Document::Ptr doc = snapshot.document(m_editor->file()->fileName());
    if (!doc)
        return;

    int line = 0;
    int column = 0;
    const int pos = m_tc.position();
    m_editor->convertPosition(pos, &line, &column);

    if (!matchDiagnosticMessage(doc, line)) {
        if (!matchIncludeFile(doc, line) && !matchMacroInUse(doc, pos)) {
            moveCursorToEndOfName(&m_tc);

            // Fetch the expression's code
            ExpressionUnderCursor expressionUnderCursor;
            const QString &expression = expressionUnderCursor(m_tc);
            Scope *scope = doc->scopeAt(line, column);

            TypeOfExpression typeOfExpression;
            typeOfExpression.init(doc, snapshot);
            const QList<LookupItem> &lookupItems = typeOfExpression(expression, scope);
            if (lookupItems.isEmpty())
                return;

            const LookupItem &lookupItem = lookupItems.first(); // ### TODO: select best candidate.
            handleLookupItemMatch(snapshot, lookupItem, typeOfExpression.context());
        }
    }
}

bool CppElementEvaluator::matchDiagnosticMessage(const CPlusPlus::Document::Ptr &document,
                                                 unsigned line)
{
    foreach (const Document::DiagnosticMessage &m, document->diagnosticMessages()) {
        if (m.line() == line) {
            m_element = QSharedPointer<CppElement>(new CppDiagnosis(m));
            return true;
        }
    }
    return false;
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
            const QString &name = use.macro().name();
            if (pos < begin + name.length()) {
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
        } else if (declaration->isClass() || declaration->isForwardClassDeclaration()) {
            if (declaration->isForwardClassDeclaration())
                if (Symbol *classDeclaration = snapshot.findMatchingClassDeclaration(declaration))
                    declaration = classDeclaration;
            CppClass *cppClass = new CppClass(declaration);
            if (m_lookupBaseClasses)
                cppClass->lookupBases(declaration, context);
            m_element = QSharedPointer<CppElement>(cppClass);
        } else if (declaration->isEnum() || declaration->enclosingScope()->isEnum()) {
            m_element = QSharedPointer<CppElement>(new CppEnum(declaration));
        } else if (declaration->isTypedef()) {
            m_element = QSharedPointer<CppElement>(new CppTypedef(declaration));
        } else if (declaration->isFunction() || (type.isValid() && type->isFunctionType())) {
            m_element = QSharedPointer<CppElement>(new CppFunction(declaration));
        } else if (declaration->isDeclaration() && type.isValid()) {
            m_element = QSharedPointer<CppElement>(
                new CppVariable(declaration, context, lookupItem.scope()));
        } else {
            m_element = QSharedPointer<CppElement>(new CppDeclarableElement(declaration));
        }
    }
}

// CppElement
CppElement::CppElement() : m_helpCategory(TextEditor::HelpItem::Unknown)
{}

CppElement::~CppElement()
{}

void CppElement::setHelpCategory(const TextEditor::HelpItem::Category &cat)
{ m_helpCategory = cat; }

const TextEditor::HelpItem::Category &CppElement::helpCategory() const
{ return m_helpCategory; }

void CppElement::setHelpIdCandidates(const QStringList &candidates)
{ m_helpIdCandidates = candidates; }

void CppElement::addHelpIdCandidate(const QString &candidate)
{ m_helpIdCandidates.append(candidate); }

const QStringList &CppElement::helpIdCandidates() const
{ return m_helpIdCandidates; }

void CppElement::setHelpMark(const QString &mark)
{ m_helpMark = mark; }

const QString &CppElement::helpMark() const
{ return m_helpMark; }

void CppElement::setLink(const CPPEditor::Link &link)
{ m_link = link; }

const CPPEditor::Link &CppElement::link() const
{ return m_link; }

void CppElement::setTooltip(const QString &tooltip)
{ m_tooltip = tooltip; }

const QString &CppElement::tooltip() const
{ return m_tooltip; }


// Unknown
Unknown::Unknown(const QString &type) : CppElement(), m_type(type)
{
    setTooltip(m_type);
}

Unknown::~Unknown()
{}

const QString &Unknown::type() const
{ return m_type; }

// CppDiagnosis
CppDiagnosis::CppDiagnosis(const Document::DiagnosticMessage &message) :
    CppElement(), m_text(message.text())
{
    setTooltip(m_text);
}

CppDiagnosis::~CppDiagnosis()
{}

const QString &CppDiagnosis::text() const
{ return m_text; }

// CppInclude
CppInclude::~CppInclude()
{}

CppInclude::CppInclude(const Document::Include &includeFile) :
    CppElement(),
    m_path(QDir::toNativeSeparators(includeFile.fileName())),
    m_fileName(QFileInfo(includeFile.fileName()).fileName())
{
    setHelpCategory(TextEditor::HelpItem::Brief);
    setHelpIdCandidates(QStringList(m_fileName));
    setHelpMark(m_fileName);
    setLink(CPPEditor::Link(m_path));
    setTooltip(m_path);
}

const QString &CppInclude::path() const
{ return m_path; }

const QString &CppInclude::fileName() const
{ return m_fileName; }

// CppMacro
CppMacro::CppMacro(const Macro &macro) : CppElement()
{
    setHelpCategory(TextEditor::HelpItem::Macro);
    setHelpIdCandidates(QStringList(macro.name()));
    setHelpMark(macro.name());
    setLink(CPPEditor::Link(macro.fileName(), macro.line()));
    setTooltip(macro.toString());
}

CppMacro::~CppMacro()
{}

// CppDeclarableElement
CppDeclarableElement::CppDeclarableElement(Symbol *declaration) : CppElement()
{
    const FullySpecifiedType &type = declaration->type();

    Overview overview;
    overview.setShowArgumentNames(true);
    overview.setShowReturnTypes(true);

    m_icon = Icons().iconForSymbol(declaration);
    m_name = overview.prettyName(declaration->name());
    if (declaration->enclosingScope()->isClass() ||
        declaration->enclosingScope()->isNamespace() ||
        declaration->enclosingScope()->isEnum()) {
        m_qualifiedName = overview.prettyName(LookupContext::fullyQualifiedName(declaration));
    } else {
        m_qualifiedName = m_name;
    }

    if (declaration->isClass() ||
        declaration->isNamespace() ||
        declaration->isForwardClassDeclaration() ||
        declaration->isEnum()) {
        m_type = m_qualifiedName;
    } else {
        m_type = overview.prettyType(type, m_qualifiedName);
    }

    setTooltip(m_type);
    setLink(CPPEditor::linkToSymbol(declaration));

    QStringList helpIds;
    helpIds << m_name << m_qualifiedName;
    setHelpIdCandidates(helpIds);
    setHelpMark(m_name);
}

CppDeclarableElement::~CppDeclarableElement()
{}

void CppDeclarableElement::setName(const QString &name)
{ m_name = name; }

const QString &CppDeclarableElement::name() const
{ return m_name; }

void CppDeclarableElement::setQualifiedName(const QString &name)
{ m_qualifiedName = name; }

const QString &CppDeclarableElement::qualifiedName() const
{ return m_qualifiedName; }

void CppDeclarableElement::setType(const QString &type)
{ m_type = type; }

const QString &CppDeclarableElement::type() const
{ return m_type; }

void CppDeclarableElement::setIcon(const QIcon &icon)
{ m_icon = icon; }

const QIcon &CppDeclarableElement::icon() const
{ return m_icon; }

// CppNamespace
CppNamespace::CppNamespace(Symbol *declaration) : CppDeclarableElement(declaration)
{
    setHelpCategory(TextEditor::HelpItem::ClassOrNamespace);
}

CppNamespace::~CppNamespace()
{}

// CppClass
CppClass::CppClass(Symbol *declaration) : CppDeclarableElement(declaration)
{
    setHelpCategory(TextEditor::HelpItem::ClassOrNamespace);
}

CppClass::~CppClass()
{}

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
                        cppClass->m_bases.append(baseCppClass);
                        q.enqueue(qMakePair(clazz, &cppClass->m_bases.last()));
                    }
                }
            }
        }
    }
}

const QList<CppClass> &CppClass::bases() const
{ return m_bases; }

// CppFunction
CppFunction::CppFunction(Symbol *declaration) : CppDeclarableElement(declaration)
{
    setHelpCategory(TextEditor::HelpItem::Function);

    const FullySpecifiedType &type = declaration->type();

    // Functions marks can be found either by the main overload or signature based
    // (with no argument names and no return). Help ids have no signature at all.
    Overview overview;
    overview.setShowDefaultArguments(false);
    setHelpMark(overview.prettyType(type, name()));

    overview.setShowFunctionSignatures(false);
    addHelpIdCandidate(overview.prettyName(declaration->name()));
}

CppFunction::~CppFunction()
{}

// CppEnum
CppEnum::CppEnum(Symbol *declaration) : CppDeclarableElement(declaration)
{
    setHelpCategory(TextEditor::HelpItem::Enum);

    if (declaration->enclosingScope()->isEnum()) {
        Symbol *enumSymbol = declaration->enclosingScope()->asEnum();
        Overview overview;
        setHelpMark(overview.prettyName(enumSymbol->name()));
        setTooltip(overview.prettyName(LookupContext::fullyQualifiedName(enumSymbol)));
    }
}

CppEnum::~CppEnum()
{}

// CppTypedef
CppTypedef::CppTypedef(Symbol *declaration) :
    CppDeclarableElement(declaration)
{
    setHelpCategory(TextEditor::HelpItem::Typedef);
}

CppTypedef::~CppTypedef()
{}

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
                setTooltip(name);
                setHelpCategory(TextEditor::HelpItem::ClassOrNamespace);
                setHelpMark(name);
                setHelpIdCandidates(QStringList(name));
            }
        }
    }
}

CppVariable::~CppVariable()
{}
