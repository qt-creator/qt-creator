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

#include "qmljseditor.h"
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

    QmlJSTextEditor *qmlEditor = qobject_cast<QmlJSTextEditor *>(editor->widget());
    if (!qmlEditor)
        return;

    if (!matchDiagnosticMessage(qmlEditor, pos)) {
        const SemanticInfo &semanticInfo = qmlEditor->semanticInfo();
        if (semanticInfo.revision() != qmlEditor->editorRevision())
            return;

        QList<AST::Node *> astPath = semanticInfo.astPath(pos);
        if (astPath.isEmpty())
            return;

        const Document::Ptr qmlDocument = semanticInfo.document;
        LookupContext::Ptr lookupContext = semanticInfo.lookupContext(astPath);

        if (!matchColorItem(lookupContext, qmlDocument, astPath, pos)) {
            handleOrdinaryMatch(lookupContext, semanticInfo.nodeUnderCursor(pos));
            const QString &helpId = qmlHelpId(toolTip());
            if (!helpId.isEmpty())
                setLastHelpItemIdentified(TextEditor::HelpItem(helpId, TextEditor::HelpItem::QML));
        }
    }
}

bool HoverHandler::matchDiagnosticMessage(QmlJSTextEditor *qmlEditor, int pos)
{
    foreach (const QTextEdit::ExtraSelection &sel,
             qmlEditor->extraSelections(TextEditor::BaseTextEditor::CodeWarningsSelection)) {
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
        if (publicMember->name && posIsInSource(pos, publicMember->expression)) {
            value = lookupContext->context()->lookup(publicMember->name->asString());
            if (const Interpreter::Reference *ref = value->asReference())
                value = lookupContext->context()->lookupReference(ref);
                color = textAt(qmlDocument,
                               publicMember->expression->firstSourceLocation(),
                               publicMember->expression->lastSourceLocation());
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
        bool found = false;
        do {
            const QString className = objectValue->className();

            if (! className.isEmpty()) {
                found = !qmlHelpId(className).isEmpty();
                if (toolTip().isEmpty() || found)
                    setToolTip(className);
            }

            objectValue = objectValue->prototype(context);
        } while (objectValue && !found);
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

QString HoverHandler::qmlHelpId(const QString &itemName) const
{
    QString helpId(QLatin1String("QML.") + itemName);
    if (!Core::HelpManager::instance()->linksForIdentifier(helpId).isEmpty())
        return helpId;
    return QString();
}
