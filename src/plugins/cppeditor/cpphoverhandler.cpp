/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "cpphoverhandler.h"
#include "cppeditor.h"
#include "cppplugin.h"

#include <coreplugin/icore.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <cpptools/cppmodelmanagerinterface.h>
#include <texteditor/itexteditor.h>
#include <texteditor/basetexteditor.h>
#include <debugger/debuggerconstants.h>

#include <CoreTypes.h>
#include <FullySpecifiedType.h>
#include <Literals.h>
#include <Names.h>
#include <Scope.h>
#include <Symbol.h>
#include <Symbols.h>
#include <cplusplus/ExpressionUnderCursor.h>
#include <cplusplus/Overview.h>
#include <cplusplus/TypeOfExpression.h>
#include <cplusplus/SimpleLexer.h>

#include <QtGui/QToolTip>
#include <QtGui/QTextCursor>
#include <QtGui/QTextBlock>
#include <QtHelp/QHelpEngineCore>
#include <QtCore/QtCore>

using namespace CppEditor::Internal;
using namespace CPlusPlus;

CppHoverHandler::CppHoverHandler(QObject *parent)
    : QObject(parent)
    , m_core(CppPlugin::core())
    , m_helpEngineNeedsSetup(false)
{
    m_modelManager = m_core->pluginManager()->getObject<CppTools::CppModelManagerInterface>();

    QFileInfo fi(ExtensionSystem::PluginManager::instance()->getObject<Core::ICore>()->settings()->fileName());
    // FIXME shouldn't the help engine create the directory if it doesn't exist?
    QDir directory(fi.absolutePath()+"/qtcreator");
    if (!directory.exists())
        directory.mkpath(directory.absolutePath());

    m_helpEngine = new QHelpEngineCore(directory.absolutePath()
                                       + QLatin1String("/helpcollection.qhc"), this);
    //m_helpEngine->setAutoSaveFilter(false);
    if (!m_helpEngine->setupData())
        qWarning() << "Could not initialize help engine:" << m_helpEngine->error();
    m_helpEngine->setCurrentFilter(tr("Unfiltered"));
    m_helpEngineNeedsSetup = m_helpEngine->registeredDocumentations().count() == 0;

    // Listen for editor opened events in order to connect to tooltip/helpid requests
    connect(m_core->editorManager(), SIGNAL(editorOpened(Core::IEditor *)),
            this, SLOT(editorOpened(Core::IEditor *)));
}

void CppHoverHandler::updateContextHelpId(TextEditor::ITextEditor *editor, int pos)
{
    updateHelpIdAndTooltip(editor, pos);
}

void CppHoverHandler::editorOpened(Core::IEditor *editor)
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

    const int dbgcontext = m_core->uniqueIDManager()->uniqueIdentifier(Debugger::Constants::C_GDBDEBUGGER);

    if (m_core->hasContext(dbgcontext))
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

static QString buildHelpId(const FullySpecifiedType &type,
                           const Symbol *symbol)
{
    Name *name = 0;
    Scope *scope = 0;

    if (const Function *f = type->asFunction()) {
        name = f->name();
        scope = f->scope();
    } else if (const Class *c = type->asClass()) {
        name = c->name();
        scope = c->scope();
    } else if (const Enum *e = type->asEnum()) {
        name = e->name();
        scope = e->scope();
    } else if (const NamedType *t = type->asNamedType()) {
        name = t->name();
    } else if (const Declaration *d = symbol->asDeclaration()) {
        if (d->scope() && d->scope()->owner()->isEnum()) {
            name = d->name();
            scope = d->scope();
        }
    }

    Overview overview;
    overview.setShowArgumentNames(false);
    overview.setShowReturnTypes(false);

    QStringList qualifiedNames;
    qualifiedNames.prepend(overview.prettyName(name));

    for (; scope; scope = scope->enclosingScope()) {
        if (scope->owner() && scope->owner()->name() && !scope->isEnumScope()) {
            Name *name = scope->owner()->name();
            Identifier *id = 0;
            if (NameId *nameId = name->asNameId()) {
                id = nameId->identifier();
            } else if (TemplateNameId *nameId = name->asTemplateNameId()) {
                id = nameId->identifier();
            }
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

    const Snapshot documents = m_modelManager->snapshot();
    const QString fileName = editor->file()->fileName();
    Document::Ptr doc = documents.value(fileName);
    if (! doc)
        return; // nothing to do

    QTextCursor tc(edit->document());
    tc.setPosition(pos);
    const unsigned lineNumber = tc.block().blockNumber() + 1;

    // Find the last symbol up to the cursor position
    int line = 0, column = 0;
    editor->convertPosition(tc.position(), &line, &column);
    Symbol *lastSymbol = doc->findSymbolAt(line, column);

    TypeOfExpression typeOfExpression;
    typeOfExpression.setSnapshot(documents);

    foreach (Document::DiagnosticMessage m, doc->diagnosticMessages()) {
        if (m.line() == lineNumber) {
            m_toolTip = m.text();
            break;
        }
    }

    if (m_toolTip.isEmpty()) {
        foreach (const Document::Include &incl, doc->includes()) {
            if (incl.line() == lineNumber) {
                m_toolTip = incl.fileName();
                break;
            }
        }
    }

    if (m_toolTip.isEmpty()) {
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

        // Fetch the expression's code.
        ExpressionUnderCursor expressionUnderCursor;
        const QString expression = expressionUnderCursor(tc);

        const QList<TypeOfExpression::Result> types =
                typeOfExpression(expression, doc, lastSymbol);

        if (!types.isEmpty()) {
            FullySpecifiedType firstType = types.first().first;
            FullySpecifiedType docType = firstType;

            if (const PointerType *pt = firstType->asPointerType()) {
                docType = pt->elementType();
            } else if (const ReferenceType *rt = firstType->asReferenceType()) {
                docType = rt->elementType();
            }

            m_helpId = buildHelpId(docType, types.first().second);
            QString displayName = buildHelpId(firstType, types.first().second);

            if (!firstType->isClass() && !firstType->isNamedType()) {
                Overview overview;
                overview.setShowArgumentNames(true);
                overview.setShowReturnTypes(true);
                m_toolTip = overview.prettyType(firstType, displayName);
            } else {
                m_toolTip = m_helpId;
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

    if (m_helpEngineNeedsSetup
        && m_helpEngine->registeredDocumentations().count() > 0) {
        m_helpEngine->setupData();
        m_helpEngineNeedsSetup = false;
    }

    if (! m_toolTip.isEmpty())
        m_toolTip = Qt::escape(m_toolTip);

    if (!m_helpId.isEmpty() && !m_helpEngine->linksForIdentifier(m_helpId).isEmpty()) {
        m_toolTip = QString(QLatin1String("<table><tr><td valign=middle><nobr>%1</td>"
                                          "<td><img src=\":/cppeditor/images/f1.svg\"></td></tr></table>"))
                    .arg(m_toolTip);
        editor->setContextHelpId(m_helpId);
    } else if (!m_toolTip.isEmpty()) {
        m_toolTip = QString(QLatin1String("<nobr>%1")).arg(m_toolTip);
    }
}
