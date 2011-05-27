/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "qmljseditor.h"
#include "qmljseditoreditable.h"
#include "qmlexpressionundercursor.h"
#include "qmljshoverhandler.h"

#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/helpmanager.h>
#include <extensionsystem/pluginmanager.h>
#include <qmljs/qmljsinterpreter.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/parser/qmljsastfwd_p.h>
#include <qmljs/qmljscheck.h>
#include <texteditor/itexteditor.h>
#include <texteditor/basetexteditor.h>
#include <texteditor/helpitem.h>
#include <texteditor/tooltip/tooltip.h>
#include <texteditor/tooltip/tipcontents.h>

#include <QtCore/QList>

using namespace Core;
using namespace QmlJS;
using namespace QmlJSEditor;
using namespace QmlJSEditor::Internal;

namespace {

    QString textAt(const Document::Ptr doc,
                   const AST::SourceLocation &from,
                   const AST::SourceLocation &to)
    {
        return doc->source().mid(from.offset, to.end() - from.begin());
    }

    AST::UiObjectInitializer *nodeInitializer(AST::Node *node)
    {
        AST::UiObjectInitializer *initializer = 0;
        if (const AST::UiObjectBinding *binding = AST::cast<const AST::UiObjectBinding *>(node))
            initializer = binding->initializer;
         else if (const AST::UiObjectDefinition *definition =
                  AST::cast<const AST::UiObjectDefinition *>(node))
            initializer = definition->initializer;
        return initializer;
    }

    template <class T>
    bool posIsInSource(const unsigned pos, T *node)
    {
        if (node &&
            pos >= node->firstSourceLocation().begin() && pos < node->lastSourceLocation().end()) {
            return true;
        }
        return false;
    }
}

HoverHandler::HoverHandler(QObject *parent) : BaseHoverHandler(parent), m_modelManager(0)
{
    m_modelManager =
        ExtensionSystem::PluginManager::instance()->getObject<QmlJS::ModelManagerInterface>();
}

bool HoverHandler::acceptEditor(IEditor *editor)
{
    QmlJSEditorEditable *qmlEditor = qobject_cast<QmlJSEditorEditable *>(editor);
    if (qmlEditor)
        return true;
    return false;
}

void HoverHandler::identifyMatch(TextEditor::ITextEditor *editor, int pos)
{
    reset();

    if (!m_modelManager)
        return;

    QmlJSEditor::QmlJSTextEditorWidget *qmlEditor = qobject_cast<QmlJSEditor::QmlJSTextEditorWidget *>(editor->widget());
    if (!qmlEditor)
        return;

    if (matchDiagnosticMessage(qmlEditor, pos))
        return;

    const QmlJSEditor::SemanticInfo &semanticInfo = qmlEditor->semanticInfo();
    if (! semanticInfo.isValid() || semanticInfo.revision() != qmlEditor->editorRevision())
        return;

    QList<AST::Node *> astPath = semanticInfo.astPath(pos);

    const Document::Ptr qmlDocument = semanticInfo.document;
    LookupContext::Ptr lookupContext = semanticInfo.lookupContext(astPath);

    AST::Node *node = semanticInfo.nodeUnderCursor(pos);
    if (astPath.isEmpty()) {
        if (AST::UiImport *import = AST::cast<AST::UiImport *>(node))
            handleImport(lookupContext, import);
        return;
    }
    if (matchColorItem(lookupContext, qmlDocument, astPath, pos))
        return;

    handleOrdinaryMatch(lookupContext, node);

    TextEditor::HelpItem helpItem = qmlHelpItem(lookupContext, node);
    if (!helpItem.helpId().isEmpty())
        setLastHelpItemIdentified(helpItem);
}

bool HoverHandler::matchDiagnosticMessage(QmlJSEditor::QmlJSTextEditorWidget *qmlEditor, int pos)
{
    foreach (const QTextEdit::ExtraSelection &sel,
             qmlEditor->extraSelections(TextEditor::BaseTextEditorWidget::CodeWarningsSelection)) {
        if (pos >= sel.cursor.selectionStart() && pos <= sel.cursor.selectionEnd()) {
            setToolTip(sel.format.toolTip());
            return true;
        }
    }
    return false;
}

bool HoverHandler::matchColorItem(const LookupContext::Ptr &lookupContext,
                                  const Document::Ptr &qmlDocument,
                                  const QList<AST::Node *> &astPath,
                                  unsigned pos)
{
    AST::UiObjectInitializer *initializer = nodeInitializer(astPath.last());
    if (!initializer)
        return false;

    AST::UiObjectMember *member = 0;
    for (AST::UiObjectMemberList *list = initializer->members; list; list = list->next) {
        if (posIsInSource(pos, list->member)) {
            member = list->member;
            break;
        }
    }
    if (!member)
        return false;

    QString color;
    const Interpreter::Value *value = 0;
    if (const AST::UiScriptBinding *binding = AST::cast<const AST::UiScriptBinding *>(member)) {
        if (binding->qualifiedId && posIsInSource(pos, binding->statement)) {
            value = lookupContext->evaluate(binding->qualifiedId);
            if (value && value->asColorValue()) {
                color = textAt(qmlDocument,
                               binding->statement->firstSourceLocation(),
                               binding->statement->lastSourceLocation());
            }
        }
    } else if (const AST::UiPublicMember *publicMember =
               AST::cast<const AST::UiPublicMember *>(member)) {
        if (publicMember->name && posIsInSource(pos, publicMember->statement)) {
            value = lookupContext->context()->lookup(publicMember->name->asString());
            if (const Interpreter::Reference *ref = value->asReference())
                value = lookupContext->context()->lookupReference(ref);
                color = textAt(qmlDocument,
                               publicMember->statement->firstSourceLocation(),
                               publicMember->statement->lastSourceLocation());
        }
    }

    if (!color.isEmpty()) {
        color.remove(QLatin1Char('\''));
        color.remove(QLatin1Char('\"'));
        color.remove(QLatin1Char(';'));

        m_colorTip = QmlJS::toQColor(color);
        if (m_colorTip.isValid()) {
            setToolTip(color);
            return true;
        }
    }
    return false;
}

void HoverHandler::handleOrdinaryMatch(const LookupContext::Ptr &lookupContext, AST::Node *node)
{
    if (node && !(AST::cast<AST::StringLiteral *>(node) != 0 ||
                  AST::cast<AST::NumericLiteral *>(node) != 0)) {
        const Interpreter::Value *value = lookupContext->evaluate(node);
        prettyPrintTooltip(value, lookupContext->context());
    }
}

void HoverHandler::handleImport(const LookupContext::Ptr &lookupContext, AST::UiImport *node)
{
    const Interpreter::Imports *imports = lookupContext->context()->imports(lookupContext->document().data());
    if (!imports)
        return;

    foreach (const Interpreter::Import &import, imports->all()) {
        if (import.info.ast() == node) {
            if (import.info.type() == Interpreter::ImportInfo::LibraryImport
                    && !import.libraryPath.isEmpty()) {
                QString msg = tr("Library at %1").arg(import.libraryPath);
                const LibraryInfo &libraryInfo = lookupContext->snapshot().libraryInfo(import.libraryPath);
                if (libraryInfo.pluginTypeInfoStatus() == LibraryInfo::DumpDone) {
                    msg += QLatin1Char('\n');
                    msg += tr("Dumped plugins successfully.");
                } else if (libraryInfo.pluginTypeInfoStatus() == LibraryInfo::TypeInfoFileDone) {
                    msg += QLatin1Char('\n');
                    msg += tr("Read typeinfo files successfully.");
                }
                setToolTip(msg);
            } else {
                setToolTip(import.info.name());
            }
            break;
        }
    }
}

void HoverHandler::reset()
{
    m_colorTip = QColor();
}

void HoverHandler::operateTooltip(TextEditor::ITextEditor *editor, const QPoint &point)
{
    if (toolTip().isEmpty())
        TextEditor::ToolTip::instance()->hide();
    else {
        if (m_colorTip.isValid()) {
            TextEditor::ToolTip::instance()->show(point,
                                                  TextEditor::ColorContent(m_colorTip),
                                                  editor->widget());
        } else {
            TextEditor::ToolTip::instance()->show(point,
                                                  TextEditor::TextContent(toolTip()),
                                                  editor->widget());
        }
    }
}

void HoverHandler::prettyPrintTooltip(const QmlJS::Interpreter::Value *value,
                                      const QmlJS::Interpreter::Context *context)
{
    if (! value)
        return;

    if (const Interpreter::ObjectValue *objectValue = value->asObjectValue()) {
        Interpreter::PrototypeIterator iter(objectValue, context);
        while (iter.hasNext()) {
            const Interpreter::ObjectValue *prototype = iter.next();
            const QString className = prototype->className();

            if (! className.isEmpty()) {
                setToolTip(className);
                break;
            }
        }
    } else if (const Interpreter::QmlEnumValue *enumValue =
               dynamic_cast<const Interpreter::QmlEnumValue *>(value)) {
        setToolTip(enumValue->name());
    }

    if (toolTip().isEmpty()) {
        QString typeId = context->engine()->typeId(value);
        if (typeId != QLatin1String("undefined"))
            setToolTip(typeId);
    }
}

// if node refers to a property, its name and defining object are returned - otherwise zero
static const Interpreter::ObjectValue *isMember(const LookupContext::Ptr &lookupContext,
                                                AST::Node *node, QString *name)
{
    const Interpreter::ObjectValue *owningObject = 0;
    if (AST::IdentifierExpression *identExp = AST::cast<AST::IdentifierExpression *>(node)) {
        if (!identExp->name)
            return 0;
        *name = identExp->name->asString();
        lookupContext->context()->lookup(*name, &owningObject);
    } else if (AST::FieldMemberExpression *fme = AST::cast<AST::FieldMemberExpression *>(node)) {
        if (!fme->base || !fme->name)
            return 0;
        *name = fme->name->asString();
        const Interpreter::Value *base = lookupContext->evaluate(fme->base);
        if (!base)
            return 0;
        owningObject = base->asObjectValue();
        if (owningObject)
            owningObject->lookupMember(*name, lookupContext->context(), &owningObject);
    } else if (AST::UiQualifiedId *qid = AST::cast<AST::UiQualifiedId *>(node)) {
        if (!qid->name)
            return 0;
        *name = qid->name->asString();
        const Interpreter::Value *value = lookupContext->context()->lookup(*name, &owningObject);
        for (AST::UiQualifiedId *it = qid->next; it; it = it->next) {
            if (!value)
                return 0;
            const Interpreter::ObjectValue *next = value->asObjectValue();
            if (!next || !it->name)
                return 0;
            *name = it->name->asString();
            value = next->lookupMember(*name, lookupContext->context(), &owningObject);
        }
    }
    return owningObject;
}

TextEditor::HelpItem HoverHandler::qmlHelpItem(const LookupContext::Ptr &lookupContext,
                                               AST::Node *node) const
{
    QString name;
    if (const Interpreter::ObjectValue *scope = isMember(lookupContext, node, &name)) {
        // maybe it's a type?
        if (!name.isEmpty() && name.at(0).isUpper()) {
            const QString maybeHelpId(QLatin1String("QML.") + name);
            if (!Core::HelpManager::instance()->linksForIdentifier(maybeHelpId).isEmpty())
                return TextEditor::HelpItem(maybeHelpId, name, TextEditor::HelpItem::QmlComponent);
        }

        // otherwise, it's probably a property
        const Interpreter::ObjectValue *lastScope;
        scope->lookupMember(name, lookupContext->context(), &lastScope);
        Interpreter::PrototypeIterator iter(scope, lookupContext->context());
        while (iter.hasNext()) {
            const Interpreter::ObjectValue *cur = iter.next();

            const QString className = cur->className();
            if (!className.isEmpty()) {
                const QString maybeHelpId(className + QLatin1String("::") + name);
                if (!Core::HelpManager::instance()->linksForIdentifier(maybeHelpId).isEmpty())
                    return TextEditor::HelpItem(maybeHelpId, name, TextEditor::HelpItem::QmlProperty);
            }

            if (cur == lastScope)
                break;
        }
    }

    return TextEditor::HelpItem();
}
