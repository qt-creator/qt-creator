// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once


#include <abstractview.h>
#include <qmldesignercomponents_global.h>

#include <coreplugin/icontext.h>

#include <memory>

namespace TextEditor { class BaseTextEditor; }

namespace Utils { class CrumblePath; }

namespace QmlDesigner {

namespace Internal {
class TextEditorContext;
}

class TextEditorWidget;

class QMLDESIGNERCOMPONENTS_EXPORT TextEditorView : public AbstractView
{
    Q_OBJECT

public:
    TextEditorView(ExternalDependenciesInterface &externalDependencies);
    ~TextEditorView() override;

    // AbstractView
    void modelAttached(Model *model) override;
    void modelAboutToBeDetached(Model *model) override;

    void importsChanged(const Imports &addedImports, const Imports &removedImports) override;

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

    void qmlJSEditorContextHelp(const Core::IContext::HelpCallback &callback) const;

    TextEditor::BaseTextEditor *textEditor();

    bool changeToMoveTool();
    bool changeToMoveTool(const QPointF &beginPoint);
    void changeToDragTool();
    void changeToSelectionTool();
    void changeToResizeTool();
    void changeToTransformTools();
    void changeToCustomTool();

    void auxiliaryDataChanged(const ModelNode &node,
                              AuxiliaryDataKeyView key,
                              const QVariant &data) override;

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
    QPointer<TextEditorWidget> m_widget;
    Internal::TextEditorContext *m_textEditorContext;
    bool m_errorState = false;
};

} // namespace QmlDesigner
