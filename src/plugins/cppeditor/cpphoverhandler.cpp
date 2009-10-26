/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
#include <QtHelp/QHelpEngineCore>

using namespace CppEditor::Internal;
using namespace CPlusPlus;
using namespace Core;

CppHoverHandler::CppHoverHandler(QObject *parent)
    : QObject(parent)
    , m_helpEngineNeedsSetup(false)
{
    m_modelManager = ExtensionSystem::PluginManager::instance()->getObject<CppTools::CppModelManagerInterface>();

    ICore *core = ICore::instance();
    QFileInfo fi(core->settings()->fileName());
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
    connect(core->editorManager(), SIGNAL(editorOpened(Core::IEditor *)),
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
    const int dbgcontext = core->uniqueIDManager()->uniqueIdentifier(Debugger::Constants::C_GDBDEBUGGER);

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

static QString buildHelpId(Symbol *symbol, Name *name)
{
    Scope *scope = 0;

    if (symbol) {
        scope = symbol->scope();
        name = symbol->name();
    }

    if (! name)
        return QString();

    Overview overview;
    overview.setShowArgumentNames(false);
    overview.setShowReturnTypes(false);

    QStringList qualifiedNames;
    qualifiedNames.prepend(overview.prettyName(name));

    for (; scope; scope = scope->enclosingScope()) {
        Symbol *owner = scope->owner();

        if (owner && owner->name() && ! scope->isEnumScope()) {
            Name *name = owner->name();
            Identifier *id = 0;

            if (NameId *nameId = name->asNameId())
                id = nameId->identifier();

            else if (TemplateNameId *nameId = name->asTemplateNameId())
                id = nameId->identifier();

            if (id)
                qualifiedNames.prepend(QString::fromLatin1(id->chars(), id->size()));
        }
    }

    return qualifiedNames.join(QLatin1String("::"));
}

// ### move me
static FullySpecifiedType resolve(const FullySpecifiedType &ty,
                                  const LookupContext &context,
                                  Symbol **resolvedSymbol,
                                  Name **resolvedName)
{
    Control *control = context.control();

    if (const PointerType *ptrTy = ty->asPointerType()) {
        return control->pointerType(resolve(ptrTy->elementType(), context,
                                            resolvedSymbol, resolvedName));

    } else if (const ReferenceType *refTy = ty->asReferenceType()) {
        return control->referenceType(resolve(refTy->elementType(), context,
                                              resolvedSymbol, resolvedName));

    } else if (const PointerToMemberType *ptrToMemTy = ty->asPointerToMemberType()) {
        return control->pointerToMemberType(ptrToMemTy->memberName(),
                                            resolve(ptrToMemTy->elementType(), context,
                                                    resolvedSymbol, resolvedName));

    } else if (const NamedType *namedTy = ty->asNamedType()) {
        if (resolvedName)
            *resolvedName = namedTy->name();

        const QList<Symbol *> candidates = context.resolve(namedTy->name());

        foreach (Symbol *c, candidates) {
            if (c->isClass() || c->isEnum()) {
                if (resolvedSymbol)
                    *resolvedSymbol = c;

                return c->type();
            }
        }

    } else if (const Namespace *nsTy = ty->asNamespaceType()) {
        if (resolvedName)
            *resolvedName = nsTy->name();

        if (resolvedSymbol)
            *resolvedSymbol = const_cast<Namespace *>(nsTy);

    } else if (const Class *classTy = ty->asClassType()) {
        if (resolvedName)
            *resolvedName = classTy->name();

        if (resolvedSymbol)
            *resolvedSymbol = const_cast<Class *>(classTy);

    } else if (const ForwardClassDeclaration *fwdClassTy = ty->asForwardClassDeclarationType()) {
        if (resolvedName)
            *resolvedName = fwdClassTy->name();

        if (resolvedSymbol)
            *resolvedSymbol = const_cast<ForwardClassDeclaration *>(fwdClassTy);

    } else if (const Enum *enumTy = ty->asEnumType()) {
        if (resolvedName)
            *resolvedName = enumTy->name();

        if (resolvedSymbol)
            *resolvedSymbol = const_cast<Enum *>(enumTy);

    } else if (const Function *funTy = ty->asFunctionType()) {
        if (resolvedName)
            *resolvedName = funTy->name();

        if (resolvedSymbol)
            *resolvedSymbol = const_cast<Function *>(funTy);

    }

    return ty;
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
    if (!doc)
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

    // We only want to show F1 if the tooltip matches the help id
    bool showF1 = true;

    foreach (Document::DiagnosticMessage m, doc->diagnosticMessages()) {
        if (m.line() == lineNumber) {
            m_toolTip = m.text();
            showF1 = false;
            break;
        }
    }

    if (m_toolTip.isEmpty()) {
        foreach (const Document::Include &incl, doc->includes()) {
            if (incl.line() == lineNumber) {
                m_toolTip = QDir::toNativeSeparators(incl.fileName());
                m_helpId = QFileInfo(incl.fileName()).fileName();
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
        ExpressionUnderCursor expressionUnderCursor;
        const QString expression = expressionUnderCursor(tc);

        const QList<TypeOfExpression::Result> types =
                typeOfExpression(expression, doc, lastSymbol);

        if (!types.isEmpty()) {
            const TypeOfExpression::Result result = types.first();

            FullySpecifiedType firstType = result.first; // result of `type of expression'.
            Symbol *lookupSymbol = result.second;        // lookup symbol

            Symbol *resolvedSymbol = lookupSymbol;
            Name *resolvedName = lookupSymbol ? lookupSymbol->name() : 0;
            firstType = resolve(firstType, typeOfExpression.lookupContext(),
                                &resolvedSymbol, &resolvedName);

            if (resolvedSymbol && resolvedSymbol->scope()
                && resolvedSymbol->scope()->isClassScope()) {
                Class *enclosingClass = resolvedSymbol->scope()->owner()->asClass();
                if (Identifier *id = enclosingClass->identifier()) {
                    if (id->isEqualTo(resolvedSymbol->identifier()))
                        resolvedSymbol = enclosingClass;
                }
            }

            m_helpId = buildHelpId(resolvedSymbol, resolvedName);

            if (m_toolTip.isEmpty()) {
                Symbol *symbol = result.second;
                if (resolvedSymbol)
                    symbol = resolvedSymbol;

                Overview overview;
                overview.setShowArgumentNames(true);
                overview.setShowReturnTypes(true);
                overview.setShowFullyQualifiedNamed(true);

                if (symbol && symbol == resolvedSymbol && symbol->isClass()) {
                    m_toolTip = m_helpId;

                } else if (lookupSymbol && (lookupSymbol->isDeclaration() || lookupSymbol->isArgument())) {
                    m_toolTip = overview.prettyType(firstType, buildHelpId(lookupSymbol, lookupSymbol->name()));

                } else if (firstType->isClassType() || firstType->isEnumType() ||
                           firstType->isForwardClassDeclarationType()) {
                    m_toolTip = m_helpId;

                } else {
                    m_toolTip = overview.prettyType(firstType, m_helpId);

                }
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

    if (!m_toolTip.isEmpty())
        m_toolTip = Qt::escape(m_toolTip);

    if (!m_helpId.isEmpty() && !m_helpEngine->linksForIdentifier(m_helpId).isEmpty()) {
        if (showF1) {
            m_toolTip = QString(QLatin1String("<table><tr><td valign=middle><nobr>%1</td>"
                                              "<td><img src=\":/cppeditor/images/f1.svg\"></td></tr></table>"))
                        .arg(m_toolTip);
        }
        editor->setContextHelpId(m_helpId);
    } else if (!m_toolTip.isEmpty()) {
        m_toolTip = QString(QLatin1String("<nobr>%1")).arg(m_toolTip);
    }
}
