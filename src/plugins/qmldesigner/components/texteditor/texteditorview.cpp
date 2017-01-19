/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "texteditorview.h"

#include "texteditorwidget.h"

#include <designmodecontext.h>
#include <designdocument.h>
#include <designersettings.h>
#include <modelnode.h>
#include <model.h>
#include <zoomaction.h>
#include <nodeabstractproperty.h>
#include <nodelistproperty.h>
#include <qmldesignerplugin.h>

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/texteditor.h>
#include <qmljseditor/qmljseditordocument.h>
#include <qmljs/qmljsreformatter.h>

#include <utils/changeset.h>

#include <QDebug>
#include <QPair>
#include <QString>
#include <QTimer>
#include <QPointer>

namespace QmlDesigner {

TextEditorView::TextEditorView(QObject *parent)
    : AbstractView(parent)
    , m_widget(new TextEditorWidget(this))
{
    Internal::TextEditorContext *textEditorContext = new Internal::TextEditorContext(m_widget.get());
    Core::ICore::addContextObject(textEditorContext);
}

TextEditorView::~TextEditorView()
{
}

void TextEditorView::modelAttached(Model *model)
{
    Q_ASSERT(model);

    AbstractView::modelAttached(model);
    m_widget->setTextEditor(qobject_cast<TextEditor::BaseTextEditor*>(QmlDesignerPlugin::instance()->currentDesignDocument()->textEditor()->duplicate()));
}

void TextEditorView::modelAboutToBeDetached(Model *model)
{
    AbstractView::modelAboutToBeDetached(model);
}

void TextEditorView::importsChanged(const QList<Import> &/*addedImports*/, const QList<Import> &/*removedImports*/)
{
}

void TextEditorView::nodeAboutToBeRemoved(const ModelNode &/*removedNode*/)
{
}

void TextEditorView::rootNodeTypeChanged(const QString &/*type*/, int /*majorVersion*/, int /*minorVersion*/)
{
}

void TextEditorView::propertiesAboutToBeRemoved(const QList<AbstractProperty>& /*propertyList*/)
{
}

void TextEditorView::nodeReparented(const ModelNode &/*node*/, const NodeAbstractProperty &/*newPropertyParent*/, const NodeAbstractProperty &/*oldPropertyParent*/, AbstractView::PropertyChangeFlags /*propertyChange*/)
{
}

WidgetInfo TextEditorView::widgetInfo()
{
    return createWidgetInfo(m_widget.get(), 0, "TextEditor", WidgetInfo::CentralPane, 0, tr("Text Editor"), DesignerWidgetFlags::IgnoreErrors);
}

QString TextEditorView::contextHelpId() const
{
    if (m_widget->textEditor()) {
        QString contextHelpId = m_widget->textEditor()->contextHelpId();
        if (!contextHelpId.isEmpty())
            return m_widget->textEditor()->contextHelpId();
    }
    return AbstractView::contextHelpId();
}

void TextEditorView::nodeIdChanged(const ModelNode& /*node*/, const QString &/*newId*/, const QString &/*oldId*/)
{
}

void TextEditorView::selectedNodesChanged(const QList<ModelNode> &/*selectedNodeList*/,
                                          const QList<ModelNode> &/*lastSelectedNodeList*/)
{
    m_widget->jumpTextCursorToSelectedModelNode();
}

void TextEditorView::customNotification(const AbstractView * /*view*/, const QString &/*identifier*/, const QList<ModelNode> &/*nodeList*/, const QList<QVariant> &/*data*/)
{
}

void TextEditorView::documentMessagesChanged(const QList<DocumentMessage> &errors, const QList<DocumentMessage> &)
{
    if (errors.isEmpty()) {
        m_widget->clearStatusBar();
    } else {
        const DocumentMessage error = errors.first();
        m_widget->setStatusText(QString("%1 (Line: %2)").arg(error.description()).arg(error.line()));
    }
}

bool TextEditorView::changeToMoveTool()
{
    return true;
}

void TextEditorView::changeToDragTool()
{
}

bool TextEditorView::changeToMoveTool(const QPointF &/*beginPoint*/)
{
    return true;
}

void TextEditorView::changeToSelectionTool()
{
}

void TextEditorView::changeToResizeTool()
{
}

void TextEditorView::changeToTransformTools()
{
}

void TextEditorView::changeToCustomTool()
{
}

void TextEditorView::auxiliaryDataChanged(const ModelNode &/*node*/, const PropertyName &/*name*/, const QVariant &/*data*/)
{
}

void TextEditorView::instancesCompleted(const QVector<ModelNode> &/*completedNodeList*/)
{
}

void TextEditorView::instanceInformationsChanged(const QMultiHash<ModelNode, InformationName> &/*informationChangeHash*/)
{
}

void TextEditorView::instancesRenderImageChanged(const QVector<ModelNode> &/*nodeList*/)
{
}

void TextEditorView::instancesChildrenChanged(const QVector<ModelNode> &/*nodeList*/)
{
}

void TextEditorView::rewriterBeginTransaction()
{
}

void TextEditorView::rewriterEndTransaction()
{
}

void TextEditorView::gotoCursorPosition(int line, int column)
{
    if (m_widget)
        m_widget->gotoCursorPosition(line, column);
}

void TextEditorView::reformatFile()
{
    int oldLine = -1;

    if (m_widget)
        oldLine = m_widget->currentLine();

    QByteArray editorState = m_widget->textEditor()->saveState();

    DesignerSettings settings = QmlDesignerPlugin::instance()->settings();

    auto document =
            qobject_cast<QmlJSEditor::QmlJSEditorDocument *>(Core::EditorManager::instance()->currentDocument());

    /* Reformat document if we have a .ui.qml file */
    if (document
            && document->filePath().toString().endsWith(".ui.qml")
            && settings.value(DesignerSettingsKey::REFORMAT_UI_QML_FILES).toBool()) {

        const QString &newText = QmlJS::reformat(document->semanticInfo().document);
        QTextCursor tc(document->document());

        Utils::ChangeSet changeSet;
        changeSet.replace(0, document->plainText().length(), newText);
        changeSet.apply(&tc);

        m_widget->textEditor()->restoreState(editorState);

        if (m_widget)
            m_widget->gotoCursorPosition(oldLine, 0);
    }
}

void TextEditorView::instancePropertyChanged(const QList<QPair<ModelNode, PropertyName> > &/*propertyList*/)
{
}

} // namespace QmlDesigner

