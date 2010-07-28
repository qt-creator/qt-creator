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

#include <coreplugin/icore.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <cpptools/cppmodelmanagerinterface.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/itexteditor.h>
#include <texteditor/basetexteditor.h>
#include <texteditor/displaysettings.h>
#include <debugger/debuggerconstants.h>

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

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QtAlgorithms>
#include <QtCore/QStringBuilder>
#include <QtGui/QToolTip>
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

    void buildClassHierarchyHelper(Symbol *symbol,
                                   const LookupContext &context,
                                   const Overview &overview,
                                   QList<QStringList> *hierarchy) {
        if (ClassOrNamespace *classSymbol = context.lookupType(symbol)) {
            const QList<ClassOrNamespace *> &bases = classSymbol->usings();
            foreach (ClassOrNamespace *baseClass, bases) {
                const QList<Symbol *> &symbols = baseClass->symbols();
                foreach (Symbol *baseSymbol, symbols) {
                    if (baseSymbol->isClass()) {
                        const QString &qualifiedName = overview.prettyName(
                                LookupContext::fullyQualifiedName(baseSymbol));
                        if (!qualifiedName.isEmpty()) {
                            hierarchy->back().append(qualifiedName);
                            buildClassHierarchyHelper(baseSymbol, context, overview, hierarchy);
                            hierarchy->append(hierarchy->back());
                            hierarchy->back().removeLast();
                        }
                    }
                }
            }
        }
    }

    void buildClassHierarchy(Symbol *symbol,
                             const LookupContext &context,
                             const Overview &overview,
                             QList<QStringList> *hierarchy) {
        if (hierarchy->isEmpty())
            hierarchy->append(QStringList());
        buildClassHierarchyHelper(symbol, context, overview, hierarchy);
        hierarchy->removeLast();
    }

    struct ClassHierarchyComp
    {
        bool operator()(const QStringList &a, const QStringList &b)
        { return a.size() < b.size(); }
    };
}

CppHoverHandler::CppHoverHandler(QObject *parent)
    : QObject(parent), m_modelManager(0), m_matchingHelpCandidate(-1)
{
    m_modelManager =
        ExtensionSystem::PluginManager::instance()->getObject<CppTools::CppModelManagerInterface>();

    // Listen for editor opened events in order to connect to tooltip/helpid requests
    connect(ICore::instance()->editorManager(), SIGNAL(editorOpened(Core::IEditor *)),
            this, SLOT(editorOpened(Core::IEditor *)));
}

void CppHoverHandler::editorOpened(IEditor *editor)
{
    CPPEditorEditable *cppEditor = qobject_cast<CPPEditorEditable *>(editor);
    if (!cppEditor)
        return;

    connect(cppEditor, SIGNAL(tooltipRequested(TextEditor::ITextEditor*, QPoint, int)),
            this, SLOT(showToolTip(TextEditor::ITextEditor*, QPoint, int)));

    connect(cppEditor, SIGNAL(contextHelpIdRequested(TextEditor::ITextEditor*, int)),
            this, SLOT(updateContextHelpId(TextEditor::ITextEditor*, int)));
}

void CppHoverHandler::updateContextHelpId(TextEditor::ITextEditor *editor, int pos)
{
    if (!editor)
        return;

    // If the tooltip is visible and there is a help match, this match is used to update the help
    // id. Otherwise, the identification process happens.
    if (!QToolTip::isVisible() || m_matchingHelpCandidate == -1)
        identifyMatch(editor, pos);

    if (m_matchingHelpCandidate != -1)
        editor->setContextHelpId(m_helpCandidates.at(m_matchingHelpCandidate).m_helpId);
    else
        editor->setContextHelpId(QString());
}

void CppHoverHandler::showToolTip(TextEditor::ITextEditor *editor, const QPoint &point, int pos)
{
    TextEditor::BaseTextEditor *baseEditor = baseTextEditor(editor);
    if (!baseEditor)
        return;

    editor->setContextHelpId(QString());

    ICore *core = ICore::instance();
    const int dbgContext =
        core->uniqueIDManager()->uniqueIdentifier(Debugger::Constants::C_DEBUGMODE);
    if (core->hasContext(dbgContext))
        return;

    identifyMatch(editor, pos);

    if (m_toolTip.isEmpty()) {
        QToolTip::hideText();
    } else {
        if (!m_classHierarchy.isEmpty())
            generateDiagramTooltip(baseEditor->displaySettings().m_extendTooltips);
        else
            generateNormalTooltip(baseEditor->displaySettings().m_extendTooltips);

        if (m_matchingHelpCandidate != -1)
            addF1ToTooltip();

        const QPoint pnt = point - QPoint(0,
#ifdef Q_WS_WIN
        24
#else
        16
#endif
        );

        QToolTip::showText(pnt, m_toolTip);
    }
}

void CppHoverHandler::identifyMatch(TextEditor::ITextEditor *editor, int pos)
{
    resetMatchings();

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
                m_toolTip = baseEditor->extraSelectionTooltip(pos);
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

        evaluateHelpCandidates();
    }
}

bool CppHoverHandler::matchDiagnosticMessage(const CPlusPlus::Document::Ptr &document,
                                             unsigned line)
{
    foreach (const Document::DiagnosticMessage &m, document->diagnosticMessages()) {
        if (m.line() == line) {
            m_toolTip = m.text();
            return true;
        }
    }
    return false;
}

bool CppHoverHandler::matchIncludeFile(const CPlusPlus::Document::Ptr &document, unsigned line)
{
    foreach (const Document::Include &includeFile, document->includes()) {
        if (includeFile.line() == line) {
            m_toolTip = QDir::toNativeSeparators(includeFile.fileName());
            const QString &fileName = QFileInfo(includeFile.fileName()).fileName();
            m_helpCandidates.append(HelpCandidate(fileName, fileName, HelpCandidate::Brief));
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
                m_toolTip = use.macro().toString();
                m_helpCandidates.append(HelpCandidate(name, name, HelpCandidate::Macro));
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
        m_toolTip = overview.prettyType(matchingType, QString());
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
                m_toolTip = name;
            } else {
                m_toolTip = overview.prettyType(matchingType, name);
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
                        m_toolTip = name;
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
                m_helpCandidates.append(HelpCandidate(functionName, docMark, helpCategory));
            } else if (matchingDeclaration->enclosingSymbol()->isEnum()) {
                Symbol *enumSymbol = matchingDeclaration->enclosingSymbol()->asEnum();
                docMark = overview.prettyName(enumSymbol->name());
            }

            m_helpCandidates.append(HelpCandidate(name, docMark, helpCategory));
        }
    }
}

void CppHoverHandler::evaluateHelpCandidates()
{
    for (int i = 0; i < m_helpCandidates.size() && m_matchingHelpCandidate == -1; ++i) {
        if (helpIdExists(m_helpCandidates.at(i).m_helpId)) {
            m_matchingHelpCandidate = i;
        } else {
            // There are class help ids with and without qualification.
            HelpCandidate &candidate = m_helpCandidates[i];
            const QString &helpId = removeClassNameQualification(candidate.m_helpId);
            if (helpIdExists(helpId)) {
                candidate.m_helpId = helpId;
                m_matchingHelpCandidate = i;
            }
        }
    }
}

bool CppHoverHandler::helpIdExists(const QString &helpId) const
{
    QMap<QString, QUrl> helpLinks = Core::HelpManager::instance()->linksForIdentifier(helpId);
    if (!helpLinks.isEmpty())
        return true;
    return false;
}

QString CppHoverHandler::getDocContents(const bool extended)
{
    Q_ASSERT(m_matchingHelpCandidate >= 0);

    return getDocContents(m_helpCandidates.at(m_matchingHelpCandidate), extended);
}

QString CppHoverHandler::getDocContents(const HelpCandidate &help, const bool extended)
{
    if (extended)
        m_htmlDocExtractor.extractExtendedContents(1500, true);
    else
        m_htmlDocExtractor.extractFirstParagraphOnly();

    QString contents;
    QMap<QString, QUrl> helpLinks =
        Core::HelpManager::instance()->linksForIdentifier(help.m_helpId);
    foreach (const QUrl &url, helpLinks) {
        const QByteArray &html = Core::HelpManager::instance()->fileData(url);
        switch (help.m_category) {
        case HelpCandidate::Brief:
            contents = m_htmlDocExtractor.getClassOrNamespaceBrief(html, help.m_docMark);
            break;
        case HelpCandidate::ClassOrNamespace:
            contents = m_htmlDocExtractor.getClassOrNamespaceDescription(html, help.m_docMark);
            break;
        case HelpCandidate::Function:
            contents = m_htmlDocExtractor.getFunctionDescription(html, help.m_docMark);
            break;
        case HelpCandidate::Enum:
            contents = m_htmlDocExtractor.getEnumDescription(html, help.m_docMark);
            break;
        case HelpCandidate::Typedef:
            contents = m_htmlDocExtractor.getTypedefDescription(html, help.m_docMark);
            break;
        case HelpCandidate::Macro:
            contents = m_htmlDocExtractor.getMacroDescription(html, help.m_docMark);
            break;
        default:
            break;
        }

        if (!contents.isEmpty())
            break;
    }
    return contents;
}

void CppHoverHandler::generateDiagramTooltip(const bool extendTooltips)
{
    QString clazz = m_toolTip;

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
                "<td>%2</td></tr>").arg(m_toolTip).arg(directBaseClasses.at(i)));
        } else {
            diagram.append(QString(
                "<tr><td></td><td>"
                "<img src=\":/cppeditor/images/larrow.png\"></td>"
                "<td>%1</td></tr>").arg(directBaseClasses.at(i)));
        }
    }
    diagram.append(QLatin1String("</table>"));
    m_toolTip = diagram;

    if (m_matchingHelpCandidate != -1) {
        m_toolTip.append(getDocContents(extendTooltips));
    } else {
        // Look for documented base classes. Diagram the nearest one or the nearest ones (in
        // the case there are many at the same level).
        int helpLevel = 0;
        QList<int> baseClassesWithHelp;
        for (int i = 0; i < m_classHierarchy.size(); ++i) {
            const QStringList &hierarchy = m_classHierarchy.at(i);
            if (helpLevel != 0 && hierarchy.size() != helpLevel)
                break;

            const QString &name = hierarchy.last();
            if (helpIdExists(name)) {
                baseClassesWithHelp.append(i);
                if (helpLevel == 0)
                    helpLevel = hierarchy.size();
            }
        }

        if (!baseClassesWithHelp.isEmpty()) {
            // Choose the first one as the help match.
            QString base = m_classHierarchy.at(baseClassesWithHelp.at(0)).last();
            HelpCandidate help(base, base, HelpCandidate::ClassOrNamespace);
            m_helpCandidates.append(help);
            m_matchingHelpCandidate = m_helpCandidates.size() - 1;

            if (baseClassesWithHelp.size() == 1 && helpLevel == 1) {
                m_toolTip.append(getDocContents(help, extendTooltips));
            } else {
                foreach (int hierarchyIndex, baseClassesWithHelp) {
                    m_toolTip.append(QLatin1String("<p>"));
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
                    QString contents =
                        getDocContents(HelpCandidate(base, base, HelpCandidate::Brief), false);
                    if (!contents.isEmpty()) {
                        m_toolTip.append(diagram % QLatin1String("<table><tr><td>") %
                                         contents % QLatin1String("</td></tr></table>"));
                    }
                    m_toolTip.append(QLatin1String("</p>"));
                }
            }
        }
    }
}

void CppHoverHandler::generateNormalTooltip(const bool extendTooltips)
{
    if (m_matchingHelpCandidate != -1) {
        const QString &contents = getDocContents(extendTooltips);
        if (!contents.isEmpty()) {
            HelpCandidate::Category cat = m_helpCandidates.at(m_matchingHelpCandidate).m_category;
            if (cat == HelpCandidate::ClassOrNamespace)
                m_toolTip.append(contents);
            else
                m_toolTip = contents;
        } else {
            m_toolTip = Qt::escape(m_toolTip);
            m_toolTip.prepend(QLatin1String("<nobr>"));
            m_toolTip.append(QLatin1String("</nobr>"));
        }
    }
}

void CppHoverHandler::addF1ToTooltip()
{
    m_toolTip = QString(QLatin1String("<table><tr><td valign=middle>%1</td><td>&nbsp;&nbsp;"
                                      "<img src=\":/cppeditor/images/f1.png\"></td>"
                                      "</tr></table>")).arg(m_toolTip);
}

void CppHoverHandler::resetMatchings()
{
    m_matchingHelpCandidate = -1;
    m_helpCandidates.clear();
    m_toolTip.clear();
    m_classHierarchy.clear();
}

TextEditor::BaseTextEditor *CppHoverHandler::baseTextEditor(TextEditor::ITextEditor *editor)
{
    if (!editor)
        return 0;
    return qobject_cast<TextEditor::BaseTextEditor *>(editor->widget());
}
