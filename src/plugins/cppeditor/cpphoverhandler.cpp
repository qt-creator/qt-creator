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

#include "cpphoverhandler.h"
#include "cppeditor.h"

#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/editormanager.h>
#include <cpptools/cppmodelmanagerinterface.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/itexteditor.h>
#include <texteditor/basetexteditor.h>

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

#include <QtCore/QSet>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QtAlgorithms>
#include <QtCore/QStringBuilder>
#include <QtGui/QTextCursor>

#include <algorithm>

using namespace CppEditor::Internal;
using namespace CPlusPlus;
using namespace Core;

namespace {
    QString removeClassNameQualification(const QString &name) {
        const int index = name.lastIndexOf(QLatin1Char(':'));
        if (index == -1)
            return name;
        else
            return name.right(name.length() - index - 1);
    }

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

    void buildClassHierarchyHelper(ClassOrNamespace *classSymbol,
                                   const LookupContext &context,
                                   const Overview &overview,
                                   QList<QStringList> *hierarchy,
                                   QSet<ClassOrNamespace *> *visited) {
        visited->insert(classSymbol);
        const QList<ClassOrNamespace *> &bases = classSymbol->usings();
        foreach (ClassOrNamespace *baseClass, bases) {
            const QList<Symbol *> &symbols = baseClass->symbols();
            foreach (Symbol *baseSymbol, symbols) {
                if (baseSymbol->isClass() && (
                    classSymbol = context.lookupType(baseSymbol)) &&
                    !visited->contains(classSymbol)) {
                    const QString &qualifiedName = overview.prettyName(
                            LookupContext::fullyQualifiedName(baseSymbol));
                    if (!qualifiedName.isEmpty()) {
                        hierarchy->back().append(qualifiedName);
                        buildClassHierarchyHelper(classSymbol,
                                                  context,
                                                  overview,
                                                  hierarchy,
                                                  visited);
                        hierarchy->append(hierarchy->back());
                        hierarchy->back().removeLast();
                    }
                }
            }
        }
    }

    void buildClassHierarchy(Symbol *symbol,
                             const LookupContext &context,
                             const Overview &overview,
                             QList<QStringList> *hierarchy) {
        if (ClassOrNamespace *classSymbol = context.lookupType(symbol)) {
            hierarchy->append(QStringList());
            QSet<ClassOrNamespace *> visited;
            buildClassHierarchyHelper(classSymbol, context, overview, hierarchy, &visited);
            hierarchy->removeLast();
        }
    }

    struct ClassHierarchyComp
    {
        bool operator()(const QStringList &a, const QStringList &b)
        { return a.size() < b.size(); }
    };
}

CppHoverHandler::CppHoverHandler(QObject *parent) : BaseHoverHandler(parent), m_modelManager(0)
{
    m_modelManager =
        ExtensionSystem::PluginManager::instance()->getObject<CppTools::CppModelManagerInterface>();
}

bool CppHoverHandler::acceptEditor(IEditor *editor)
{
    CPPEditorEditable *cppEditor = qobject_cast<CPPEditorEditable *>(editor);
    if (cppEditor)
        return true;
    return false;
}

void CppHoverHandler::identifyMatch(TextEditor::ITextEditor *editor, int pos)
{
    if (!m_modelManager)
        return;

    const Snapshot &snapshot = m_modelManager->snapshot();
    Document::Ptr doc = snapshot.document(editor->file()->fileName());
    if (!doc)
        return;

    int line = 0;
    int column = 0;
    editor->convertPosition(pos, &line, &column);

    if (!matchDiagnosticMessage(doc, line)) {
        if (!matchIncludeFile(doc, line) && !matchMacroInUse(doc, pos)) {
            TextEditor::BaseTextEditor *baseEditor = baseTextEditor(editor);
            if (!baseEditor)
                return;

            bool extraSelectionTooltip = false;
            if (!baseEditor->extraSelectionTooltip(pos).isEmpty()) {
                setToolTip(baseEditor->extraSelectionTooltip(pos));
                extraSelectionTooltip = true;
            }

            QTextCursor tc(baseEditor->document());
            tc.setPosition(pos);
            moveCursorToEndOfName(&tc);

            // Fetch the expression's code
            ExpressionUnderCursor expressionUnderCursor;
            const QString &expression = expressionUnderCursor(tc);
            Scope *scope = doc->scopeAt(line, column);

            TypeOfExpression typeOfExpression;
            typeOfExpression.init(doc, snapshot);
            const QList<LookupItem> &lookupItems = typeOfExpression(expression, scope);
            if (lookupItems.isEmpty())
                return;

            const LookupItem &lookupItem = lookupItems.first(); // ### TODO: select best candidate.
            handleLookupItemMatch(lookupItem, typeOfExpression.context(), !extraSelectionTooltip);
        }
    }
}

bool CppHoverHandler::matchDiagnosticMessage(const CPlusPlus::Document::Ptr &document,
                                             unsigned line)
{
    foreach (const Document::DiagnosticMessage &m, document->diagnosticMessages()) {
        if (m.line() == line) {
            setToolTip(m.text());
            return true;
        }
    }
    return false;
}

bool CppHoverHandler::matchIncludeFile(const CPlusPlus::Document::Ptr &document, unsigned line)
{
    foreach (const Document::Include &includeFile, document->includes()) {
        if (includeFile.line() == line) {
            setToolTip(QDir::toNativeSeparators(includeFile.fileName()));
            const QString &fileName = QFileInfo(includeFile.fileName()).fileName();
            addHelpCandidate(HelpCandidate(fileName, fileName, HelpCandidate::Brief));
            return true;
        }
    }
    return false;
}

bool CppHoverHandler::matchMacroInUse(const CPlusPlus::Document::Ptr &document, unsigned pos)
{
    foreach (const Document::MacroUse &use, document->macroUses()) {
        if (use.contains(pos)) {
            const unsigned begin = use.begin();
            const QString &name = use.macro().name();
            if (pos < begin + name.length()) {
                setToolTip(use.macro().toString());
                addHelpCandidate(HelpCandidate(name, name, HelpCandidate::Macro));
                return true;
            }
        }
    }
    return false;
}

void CppHoverHandler::handleLookupItemMatch(const LookupItem &lookupItem,
                                            const LookupContext &context,
                                            const bool assignTooltip)
{
    Symbol *matchingDeclaration = lookupItem.declaration();
    FullySpecifiedType matchingType = lookupItem.type();

    Overview overview;
    overview.setShowArgumentNames(true);
    overview.setShowReturnTypes(true);

    if (!matchingDeclaration && assignTooltip) {
        setToolTip(overview.prettyType(matchingType, QString()));
    } else {
        QString name;
        if (matchingDeclaration->enclosingSymbol()->isClass() ||
            matchingDeclaration->enclosingSymbol()->isNamespace() ||
            matchingDeclaration->enclosingSymbol()->isEnum()) {
            name.append(overview.prettyName(
                LookupContext::fullyQualifiedName(matchingDeclaration)));

            if (matchingDeclaration->isClass() ||
                matchingDeclaration->isForwardClassDeclaration()) {
                buildClassHierarchy(matchingDeclaration, context, overview, &m_classHierarchy);
            }
        } else {
            name.append(overview.prettyName(matchingDeclaration->name()));
        }

        if (assignTooltip) {
            if (matchingDeclaration->isClass() ||
                matchingDeclaration->isNamespace() ||
                matchingDeclaration->isForwardClassDeclaration() ||
                matchingDeclaration->isEnum()) {
                setToolTip(name);
            } else {
                setToolTip(overview.prettyType(matchingType, name));
            }
        }

        HelpCandidate::Category helpCategory = HelpCandidate::Unknown;
        if (matchingDeclaration->isNamespace() ||
            matchingDeclaration->isClass() ||
            matchingDeclaration->isForwardClassDeclaration()) {
            helpCategory = HelpCandidate::ClassOrNamespace;
        } else if (matchingDeclaration->isEnum() ||
                   matchingDeclaration->enclosingSymbol()->isEnum()) {
            helpCategory = HelpCandidate::Enum;
        } else if (matchingDeclaration->isTypedef()) {
            helpCategory = HelpCandidate::Typedef;
        } else if (matchingDeclaration->isFunction() ||
                  (matchingType.isValid() && matchingType->isFunctionType())){
            helpCategory = HelpCandidate::Function;
        } else if (matchingDeclaration->isDeclaration() && matchingType.isValid()) {
            const Name *typeName = 0;
            if (matchingType->isNamedType()) {
                typeName = matchingType->asNamedType()->name();
            } else if (matchingType->isPointerType() || matchingType->isReferenceType()) {
                FullySpecifiedType type;
                if (matchingType->isPointerType())
                    type = matchingType->asPointerType()->elementType();
                else
                    type = matchingType->asReferenceType()->elementType();
                if (type->isNamedType())
                    typeName = type->asNamedType()->name();
            }

            if (typeName) {
                if (ClassOrNamespace *clazz = context.lookupType(typeName, lookupItem.scope())) {
                    if (!clazz->symbols().isEmpty()) {
                        Symbol *symbol = clazz->symbols().at(0);
                        matchingDeclaration = symbol;
                        name = overview.prettyName(LookupContext::fullyQualifiedName(symbol));
                        setToolTip(name);
                        buildClassHierarchy(symbol, context, overview, &m_classHierarchy);
                        helpCategory = HelpCandidate::ClassOrNamespace;
                    }
                }
            }
        }

        if (helpCategory != HelpCandidate::Unknown) {
            QString docMark = overview.prettyName(matchingDeclaration->name());

            if (matchingType.isValid() && matchingType->isFunctionType()) {
                // Functions marks can be found either by the main overload or signature based
                // (with no argument names and no return). Help ids have no signature at all.
                overview.setShowArgumentNames(false);
                overview.setShowReturnTypes(false);
                docMark = overview.prettyType(matchingType, docMark);
                overview.setShowFunctionSignatures(false);
                const QString &functionName = overview.prettyName(matchingDeclaration->name());
                addHelpCandidate(HelpCandidate(functionName, docMark, helpCategory));
            } else if (matchingDeclaration->enclosingSymbol()->isEnum()) {
                Symbol *enumSymbol = matchingDeclaration->enclosingSymbol()->asEnum();
                docMark = overview.prettyName(enumSymbol->name());
            }

            addHelpCandidate(HelpCandidate(name, docMark, helpCategory));
        }
    }
}

void CppHoverHandler::evaluateHelpCandidates()
{
    for (int i = 0; i < helpCandidates().size() && matchingHelpCandidate() == -1; ++i) {
        if (helpIdExists(helpCandidates().at(i).m_helpId)) {
            setMatchingHelpCandidate(i);
        } else {
            // There are class help ids with and without qualification.
            HelpCandidate candidate = helpCandidates().at(i);
            const QString &helpId = removeClassNameQualification(candidate.m_helpId);
            if (helpIdExists(helpId)) {
                candidate.m_helpId = helpId;
                setHelpCandidate(candidate, i);
                setMatchingHelpCandidate(i);
            }
        }
    }
}

void CppHoverHandler::decorateToolTip(TextEditor::ITextEditor *editor)
{
    if (!m_classHierarchy.isEmpty())
        generateDiagramTooltip(extendToolTips(editor));
    else
        generateNormalTooltip(extendToolTips(editor));
}

void CppHoverHandler::generateDiagramTooltip(const bool extendTooltips)
{
    QString clazz = toolTip();

    qSort(m_classHierarchy.begin(), m_classHierarchy.end(), ClassHierarchyComp());

    // Remove duplicates (in case there are any).
    m_classHierarchy.erase(std::unique(m_classHierarchy.begin(), m_classHierarchy.end()),
                           m_classHierarchy.end());

    QStringList directBaseClasses;
    foreach (const QStringList &hierarchy, m_classHierarchy) {
        if (hierarchy.size() > 1)
            break;
        directBaseClasses.append(hierarchy.at(0));
    }

    QString diagram(QLatin1String("<table>"));
    for (int i = 0; i < directBaseClasses.size(); ++i) {
        if (i == 0) {
            diagram.append(QString(
                "<tr><td>%1</td><td>"
                "<img src=\":/cppeditor/images/rightarrow.png\"></td>"
                "<td>%2</td></tr>").arg(toolTip()).arg(directBaseClasses.at(i)));
        } else {
            diagram.append(QString(
                "<tr><td></td><td>"
                "<img src=\":/cppeditor/images/larrow.png\"></td>"
                "<td>%1</td></tr>").arg(directBaseClasses.at(i)));
        }
    }
    diagram.append(QLatin1String("</table>"));
    setToolTip(diagram);

    if (matchingHelpCandidate() != -1) {
        appendToolTip(getDocContents(extendTooltips));
    } else {
        // Look for documented base classes. Diagram the nearest one or the nearest ones (in
        // the case there are many at the same level).
        int helpLevel = 0;
        QList<int> baseClassesWithHelp;
        for (int i = 0; i < m_classHierarchy.size(); ++i) {
            const QStringList &hierarchy = m_classHierarchy.at(i);
            if (helpLevel != 0 && hierarchy.size() != helpLevel)
                break;

            bool exists = false;
            QString name = hierarchy.last();
            if (helpIdExists(name)) {
                exists = true;
            } else {
                name = removeClassNameQualification(name);
                if (helpIdExists(name)) {
                    exists = true;
                    m_classHierarchy[i].last() = name;
                }
            }
            if (exists) {
                baseClassesWithHelp.append(i);
                if (helpLevel == 0)
                    helpLevel = hierarchy.size();
            }
        }

        if (!baseClassesWithHelp.isEmpty()) {
            // Choose the first one as the help match.
            QString base = m_classHierarchy.at(baseClassesWithHelp.at(0)).last();
            HelpCandidate help(base, base, HelpCandidate::ClassOrNamespace);
            addHelpCandidate(help);
            setMatchingHelpCandidate(helpCandidates().size() - 1);

            if (baseClassesWithHelp.size() == 1 && helpLevel == 1) {
                appendToolTip(getDocContents(help, extendTooltips));
            } else {
                foreach (int hierarchyIndex, baseClassesWithHelp) {
                    appendToolTip(QLatin1String("<p>"));
                    const QStringList &hierarchy = m_classHierarchy.at(hierarchyIndex);
                    Q_ASSERT(helpLevel <= hierarchy.size());

                    // Following contents are inside tables so they are on the exact same
                    // alignment as the top level diagram.
                    diagram = QString(QLatin1String("<table><tr><td>%1</td>")).arg(clazz);
                    for (int i = 0; i < helpLevel; ++i) {
                        diagram.append(
                            QLatin1String("<td><img src=\":/cppeditor/images/rightarrow.png\">"
                                          "</td><td>") %
                            hierarchy.at(i) %
                            QLatin1String("</td>"));
                    }
                    diagram.append(QLatin1String("</tr></table>"));

                    base = hierarchy.at(helpLevel - 1);
                    const QString &contents =
                        getDocContents(HelpCandidate(base, base, HelpCandidate::Brief), false);
                    if (!contents.isEmpty()) {
                        appendToolTip(diagram % QLatin1String("<table><tr><td>") %
                                      contents % QLatin1String("</td></tr></table>"));
                    }
                    appendToolTip(QLatin1String("</p>"));
                }
            }
        }
    }
}

void CppHoverHandler::generateNormalTooltip(const bool extendTooltips)
{
    if (matchingHelpCandidate() != -1) {
        const QString &contents = getDocContents(extendTooltips);
        if (!contents.isEmpty()) {
            HelpCandidate::Category cat = helpCandidate(matchingHelpCandidate()).m_category;
            if (cat == HelpCandidate::ClassOrNamespace)
                appendToolTip(contents);
            else
                setToolTip(contents);
        } else {
            QString tip = Qt::escape(toolTip());
            tip.prepend(QLatin1String("<nobr>"));
            tip.append(QLatin1String("</nobr>"));
            setToolTip(tip);
        }
    }
}

void CppHoverHandler::resetExtras()
{
    m_classHierarchy.clear();
}
