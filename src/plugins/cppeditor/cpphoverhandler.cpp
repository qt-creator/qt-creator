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
#include "cppplugin.h"

#include <coreplugin/icore.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <cpptools/cppmodelmanagerinterface.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/itexteditor.h>
#include <texteditor/basetexteditor.h>
#include <debugger/debuggerconstants.h>

#include <CoreTypes.h>
#include <FullySpecifiedType.h>
#include <Literals.h>
#include <Control.h>
#include <Names.h>
#include <Scope.h>
#include <Symbol.h>
#include <Symbols.h>
#include <cplusplus/ExpressionUnderCursor.h>
#include <cplusplus/Overview.h>
#include <cplusplus/TypeOfExpression.h>
#include <cplusplus/SimpleLexer.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QSettings>
#include <QtGui/QToolTip>
#include <QtGui/QTextCursor>
#include <QtGui/QTextBlock>

using namespace CppEditor::Internal;
using namespace CPlusPlus;
using namespace Core;

CppHoverHandler::CppHoverHandler(QObject *parent)
    : QObject(parent)
{
    m_modelManager = ExtensionSystem::PluginManager::instance()->getObject<CppTools::CppModelManagerInterface>();

    // Listen for editor opened events in order to connect to tooltip/helpid requests
    connect(ICore::instance()->editorManager(), SIGNAL(editorOpened(Core::IEditor *)),
            this, SLOT(editorOpened(Core::IEditor *)));
}

void CppHoverHandler::updateContextHelpId(TextEditor::ITextEditor *editor, int pos)
{
    updateHelpIdAndTooltip(editor, pos);
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

void CppHoverHandler::showToolTip(TextEditor::ITextEditor *editor, const QPoint &point, int pos)
{
    if (!editor)
        return;

    ICore *core = ICore::instance();
    const int dbgcontext = core->uniqueIDManager()->uniqueIdentifier(Debugger::Constants::C_DEBUGMODE);

    if (core->hasContext(dbgcontext))
        return;

    updateHelpIdAndTooltip(editor, pos);

    if (m_toolTip.isEmpty())
        QToolTip::hideText();
    else {
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

static QString buildHelpId(Symbol *symbol, const Name *declarationName)
{
    Scope *scope = 0;

    if (symbol) {
        scope = symbol->scope();
        declarationName = symbol->name();
    }

    if (! declarationName)
        return QString();

    Overview overview;
    overview.setShowArgumentNames(false);
    overview.setShowReturnTypes(false);

    QStringList qualifiedNames;
    qualifiedNames.prepend(overview.prettyName(declarationName));

    for (; scope; scope = scope->enclosingScope()) {
        Symbol *owner = scope->owner();

        if (owner && owner->name() && ! scope->isEnumScope()) {
            const Name *name = owner->name();
            const Identifier *id = 0;

            if (const NameId *nameId = name->asNameId())
                id = nameId->identifier();

            else if (const TemplateNameId *nameId = name->asTemplateNameId())
                id = nameId->identifier();

            if (id)
                qualifiedNames.prepend(QString::fromLatin1(id->chars(), id->size()));
        }
    }

    return qualifiedNames.join(QLatin1String("::"));
}

void CppHoverHandler::updateHelpIdAndTooltip(TextEditor::ITextEditor *editor, int pos)
{
    m_helpId.clear();
    m_toolTip.clear();

    if (!m_modelManager)
        return;

    TextEditor::BaseTextEditor *edit = qobject_cast<TextEditor::BaseTextEditor *>(editor->widget());
    if (!edit)
        return;

    const Snapshot snapshot = m_modelManager->snapshot();
    const QString fileName = editor->file()->fileName();
    Document::Ptr doc = snapshot.document(fileName);
    if (!doc)
        return; // nothing to do

    QString formatTooltip = edit->extraSelectionTooltip(pos);
    QTextCursor tc(edit->document());
    tc.setPosition(pos);

    const unsigned lineNumber = tc.block().blockNumber() + 1;

    // Find the last symbol up to the cursor position
    int line = 0, column = 0;
    editor->convertPosition(tc.position(), &line, &column);
    Scope *scope = doc->scopeAt(line, column);

    TypeOfExpression typeOfExpression;
    typeOfExpression.init(doc, snapshot);

    // We only want to show F1 if the tooltip matches the help id
    bool showF1 = true;

    foreach (const Document::DiagnosticMessage &m, doc->diagnosticMessages()) {
        if (m.line() == lineNumber) {
            m_toolTip = m.text();
            showF1 = false;
            break;
        }
    }

    QMap<QString, QUrl> helpLinks;
    if (m_toolTip.isEmpty()) {
        foreach (const Document::Include &incl, doc->includes()) {
            if (incl.line() == lineNumber) {
                m_toolTip = QDir::toNativeSeparators(incl.fileName());
                m_helpId = QFileInfo(incl.fileName()).fileName();
                helpLinks = Core::HelpManager::instance()->linksForIdentifier(m_helpId);
                break;
            }
        }
    }

    if (m_helpId.isEmpty()) {
        // Move to the end of a qualified name
        bool stop = false;
        while (!stop) {
            const QChar ch = editor->characterAt(tc.position());
            if (ch.isLetterOrNumber() || ch == QLatin1Char('_'))
                tc.setPosition(tc.position() + 1);
            else if (ch == QLatin1Char(':') && editor->characterAt(tc.position() + 1) == QLatin1Char(':')) {
                tc.setPosition(tc.position() + 2);
            } else {
                stop = true;
            }
        }

        // Fetch the expression's code
        ExpressionUnderCursor expressionUnderCursor(m_modelManager->tokenCache(editor));
        const QString expression = expressionUnderCursor(tc);

        const QList<LookupItem> types = typeOfExpression(expression, scope);


        if (!types.isEmpty()) {
            Overview overview;
            overview.setShowArgumentNames(true);
            overview.setShowReturnTypes(true);
            overview.setShowFullyQualifiedNamed(true);

            const LookupItem result = types.first(); // ### TODO: select the best candidate.
            FullySpecifiedType symbolTy = result.type(); // result of `type of expression'.
            Symbol *declaration = result.declaration(); // lookup symbol
            const Name *declarationName = declaration ? declaration->name() : 0;

            if (declaration && declaration->scope()
                && declaration->scope()->isClassScope()) {
                Class *enclosingClass = declaration->scope()->owner()->asClass();
                if (const Identifier *id = enclosingClass->identifier()) {
                    if (id->isEqualTo(declaration->identifier()))
                        declaration = enclosingClass;
                }
            }

            m_helpId = buildHelpId(declaration, declarationName);

            if (m_toolTip.isEmpty()) {
                Symbol *symbol = declaration;

                if (declaration)
                    symbol = declaration;

                if (symbol && symbol == declaration && symbol->isClass()) {
                    m_toolTip = m_helpId;

                } else if (declaration && (declaration->isDeclaration() || declaration->isArgument())) {
                    m_toolTip = overview.prettyType(symbolTy, buildHelpId(declaration, declaration->name()));

                } else if (symbolTy->isClassType() || symbolTy->isEnumType() ||
                           symbolTy->isForwardClassDeclarationType()) {
                    m_toolTip = m_helpId;

                } else {
                    m_toolTip = overview.prettyType(symbolTy, m_helpId);

                }
            }

            // Some docs don't contain the namespace in the documentation pages, for instance
            // there is QtMobility::QContactManager but the help page is for QContactManager.
            // To show their help anyway, try stripping scopes until we find something.
            const QString startHelpId = m_helpId;
            while (!m_helpId.isEmpty()) {
                helpLinks = Core::HelpManager::instance()->linksForIdentifier(m_helpId);
                if (!helpLinks.isEmpty())
                    break;

                int coloncolonIndex = m_helpId.indexOf(QLatin1String("::"));
                if (coloncolonIndex == -1) {
                    m_helpId = startHelpId;
                    break;
                }

                m_helpId.remove(0, coloncolonIndex + 2);
            }
        }
    }

    if (m_toolTip.isEmpty()) {
        foreach (const Document::MacroUse &use, doc->macroUses()) {
            if (use.contains(pos)) {
                const Macro m = use.macro();
                m_toolTip = m.toString();
                m_helpId = m.name();
                break;
            }
        }
    }

    if (!formatTooltip.isEmpty())
        m_toolTip = formatTooltip;

    if (!m_helpId.isEmpty() && !helpLinks.isEmpty()) {
        if (showF1) {
            // we need the original width without escape sequences
            const int width = QFontMetrics(QToolTip::font()).width(m_toolTip);
            m_toolTip = QString(QLatin1String("<table><tr><td valign=middle width=%2>%1</td>"
                                              "<td><img src=\":/cppeditor/images/f1.png\"></td></tr></table>"))
                        .arg(Qt::escape(m_toolTip)).arg(width);
        }
        editor->setContextHelpId(m_helpId);
    } else if (!m_toolTip.isEmpty() && Qt::mightBeRichText(m_toolTip)) {
        m_toolTip = QString(QLatin1String("<nobr>%1</nobr>")).arg(Qt::escape(m_toolTip));
    }
}
