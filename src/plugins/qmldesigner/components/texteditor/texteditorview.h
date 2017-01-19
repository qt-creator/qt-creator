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
#pragma once

#include <abstractview.h>

#include <memory>

namespace TextEditor { class BaseTextEditor; }

namespace Utils { class CrumblePath; }

namespace QmlDesigner {

namespace Internal {
class TextEditorContext;
}

class TextEditorWidget;

class QMLDESIGNERCORE_EXPORT TextEditorView : public AbstractView
{
    Q_OBJECT

public:
    TextEditorView(QObject *parent = 0);
    ~TextEditorView();

    // AbstractView
    void modelAttached(Model *model) override;
    void modelAboutToBeDetached(Model *model) override;

    void importsChanged(const QList<Import> &addedImports, const QList<Import> &removedImports) override;

    void nodeAboutToBeRemoved(const ModelNode &removedNode) override;
    void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange) override;
    void nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId) override;
    void propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList) override;
    void rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion) override;

    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                              const QList<ModelNode> &lastSelectedNodeList) override;
    void customNotification(const AbstractView *view, const QString &identifier, const QList<ModelNode> &nodeList, const QList<QVariant> &data) override;
    void documentMessagesChanged(const QList<DocumentMessage> &errors, const QList<DocumentMessage> &warnings) override;

    // TextEditorView
    WidgetInfo widgetInfo() override;
    QString contextHelpId() const override;

    TextEditor::BaseTextEditor *textEditor();

    bool changeToMoveTool();
    bool changeToMoveTool(const QPointF &beginPoint);
    void changeToDragTool();
    void changeToSelectionTool();
    void changeToResizeTool();
    void changeToTransformTools();
    void changeToCustomTool();

    void auxiliaryDataChanged(const ModelNode &node, const PropertyName &name, const QVariant &data) override;

    void instancesCompleted(const QVector<ModelNode> &completedNodeList) override;
    void instanceInformationsChanged(const QMultiHash<ModelNode, InformationName> &informationChangeHash) override;
    void instancesRenderImageChanged(const QVector<ModelNode> &nodeList) override;
    void instancesChildrenChanged(const QVector<ModelNode> &nodeList) override;
    void instancePropertyChanged(const QList<QPair<ModelNode, PropertyName> > &propertyList) override;

    void rewriterBeginTransaction() override;
    void rewriterEndTransaction() override;

    void deActivateItemCreator();

    void gotoCursorPosition(int line, int column);

    void reformatFile();

private:
    std::unique_ptr<TextEditorWidget> m_widget;
    Internal::TextEditorContext *m_textEditorContext;
};

} // namespace QmlDesigner
