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

#include <coreplugin/icore.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <debugger/debuggerconstants.h>
#include <extensionsystem/pluginmanager.h>
#include <qmljs/qmljsinterpreter.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/parser/qmljsastfwd_p.h>
#include <qmljs/qmljscheck.h>
#include <texteditor/itexteditor.h>
#include <texteditor/basetexteditor.h>
#include <texteditor/tooltip/tooltip.h>

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

HoverHandler::HoverHandler(QObject *parent) :
    QObject(parent), m_modelManager(0), m_matchingHelpCandidate(-1)
{
    m_modelManager =
        ExtensionSystem::PluginManager::instance()->getObject<QmlJS::ModelManagerInterface>();

    // Listen for editor opened events in order to connect to tooltip/helpid requests
    connect(ICore::instance()->editorManager(), SIGNAL(editorOpened(Core::IEditor *)),
            this, SLOT(editorOpened(Core::IEditor *)));
}

void HoverHandler::editorOpened(IEditor *editor)
{
    QmlJSEditorEditable *qmlEditor = qobject_cast<QmlJSEditorEditable *>(editor);
    if (!qmlEditor)
        return;

    connect(qmlEditor, SIGNAL(tooltipRequested(TextEditor::ITextEditor*, QPoint, int)),
            this, SLOT(showToolTip(TextEditor::ITextEditor*, QPoint, int)));

    connect(qmlEditor, SIGNAL(contextHelpIdRequested(TextEditor::ITextEditor*, int)),
            this, SLOT(updateContextHelpId(TextEditor::ITextEditor*, int)));
}

void HoverHandler::showToolTip(TextEditor::ITextEditor *editor, const QPoint &point, int pos)
{
    if (!editor)
        return;

    ICore *core = ICore::instance();
    const int dbgcontext =
        core->uniqueIDManager()->uniqueIdentifier(Debugger::Constants::C_DEBUGMODE);
    if (core->hasContext(dbgcontext))
        return;

    editor->setContextHelpId(QString());

    identifyMatch(editor, pos);

    if (m_toolTip.isEmpty())
        TextEditor::ToolTip::instance()->hide();
    else {
        const QPoint &pnt = point - QPoint(0,
#ifdef Q_WS_WIN
        24
#else
        16
#endif
        );

        if (m_colorTip.isValid()) {
            TextEditor::ToolTip::instance()->showColor(pnt, m_colorTip, editor->widget());
        } else {
            m_toolTip = Qt::escape(m_toolTip);
            if (m_matchingHelpCandidate != -1) {
                m_toolTip = QString::fromUtf8(
                    "<table><tr><td valign=middle><nobr>%1</td><td>"
                    "<img src=\":/cppeditor/images/f1.png\"></td></tr></table>").arg(m_toolTip);
            } else {
                m_toolTip = QString::fromUtf8("<nobr>%1</nobr>").arg(m_toolTip);
            }
            TextEditor::ToolTip::instance()->showText(pnt, m_toolTip, editor->widget());
        }
    }
}

void HoverHandler::updateContextHelpId(TextEditor::ITextEditor *editor, int pos)
{
    // If the tooltip is visible and there is a help match, use it to update the help id.
    // Otherwise, identify the match.
    if (!TextEditor::ToolTip::instance()->isVisible() || m_matchingHelpCandidate == -1)
        identifyMatch(editor, pos);

    if (m_matchingHelpCandidate != -1)
        editor->setContextHelpId(m_helpCandidates.at(m_matchingHelpCandidate));
    else
        editor->setContextHelpId(QString());
}

void HoverHandler::resetMatchings()
{
    m_matchingHelpCandidate = -1;
    m_helpCandidates.clear();
    m_toolTip.clear();
    m_colorTip = QColor();
}

void HoverHandler::identifyMatch(TextEditor::ITextEditor *editor, int pos)
{
    resetMatchings();

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

        const Snapshot &snapshot = semanticInfo.snapshot;
        const Document::Ptr qmlDocument = semanticInfo.document;
        LookupContext::Ptr lookupContext = LookupContext::create(qmlDocument, snapshot, astPath);

        if (!matchColorItem(lookupContext, qmlDocument, astPath, pos))
            handleOrdinaryMatch(lookupContext, semanticInfo.nodeUnderCursor(pos));
    }

    evaluateHelpCandidates();
}

bool HoverHandler::matchDiagnosticMessage(QmlJSTextEditor *qmlEditor, int pos)
{
    foreach (const QTextEdit::ExtraSelection &sel,
             qmlEditor->extraSelections(TextEditor::BaseTextEditor::CodeWarningsSelection)) {
        if (pos >= sel.cursor.selectionStart() && pos <= sel.cursor.selectionEnd()) {
            m_toolTip = sel.format.toolTip();
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
            m_toolTip = color;
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
        m_toolTip = prettyPrint(value, lookupContext->context());
    }
}

void HoverHandler::evaluateHelpCandidates()
{
    for (int i = 0; i < m_helpCandidates.size(); ++i) {
        QString helpId = m_helpCandidates.at(i);
        helpId.prepend(QLatin1String("QML."));
        if (!Core::HelpManager::instance()->linksForIdentifier(helpId).isEmpty()) {
            m_matchingHelpCandidate = i;
            m_helpCandidates[i] = helpId;
            break;
        }
    }
}

QString HoverHandler::prettyPrint(const QmlJS::Interpreter::Value *value,
                                  QmlJS::Interpreter::Context *context)
{
    if (! value)
        return QString();

    if (const Interpreter::ObjectValue *objectValue = value->asObjectValue()) {
        do {
            const QString className = objectValue->className();

            if (! className.isEmpty())
                m_helpCandidates.append(className);

            objectValue = objectValue->prototype(context);
        } while (objectValue);

        if (! m_helpCandidates.isEmpty())
            return m_helpCandidates.first();
    } else if (const Interpreter::QmlEnumValue *enumValue =
               dynamic_cast<const Interpreter::QmlEnumValue *>(value)) {
        return enumValue->name();
    }

    QString typeId = context->engine()->typeId(value);

    if (typeId == QLatin1String("undefined"))
        typeId.clear();

    return typeId;
}
