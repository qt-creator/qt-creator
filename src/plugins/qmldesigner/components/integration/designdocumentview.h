/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DesignDocumentVIEW_H
#define DesignDocumentVIEW_H

#include <abstractview.h>
#include <modelmerger.h>

namespace QmlDesigner {

class DesignDocumentView : public AbstractView
{
        Q_OBJECT
public:
    DesignDocumentView(QObject *parent = 0);
    ~DesignDocumentView();

    virtual void nodeCreated(const ModelNode &createdNode) override;
    virtual void nodeAboutToBeRemoved(const ModelNode &removedNode) override;
    virtual void nodeRemoved(const ModelNode &removedNode, const NodeAbstractProperty &parentProperty, PropertyChangeFlags propertyChange) override;
    virtual void nodeAboutToBeReparented(const ModelNode &node,
                                         const NodeAbstractProperty &newPropertyParent,
                                         const NodeAbstractProperty &oldPropertyParent,
                                         AbstractView::PropertyChangeFlags propertyChange)  override;
    virtual void nodeReparented(const ModelNode &node,
                                const NodeAbstractProperty &newPropertyParent,
                                const NodeAbstractProperty &oldPropertyParent,
                                AbstractView::PropertyChangeFlags propertyChange)  override;
    virtual void nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId) override;
    virtual void propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList) override;
    virtual void propertiesRemoved(const QList<AbstractProperty>& propertyList) override;
    virtual void variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChange) override;
    virtual void bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags propertyChange) override;
    virtual void signalHandlerPropertiesChanged(const QVector<SignalHandlerProperty>& propertyList, PropertyChangeFlags propertyChange) override;
    virtual void rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion) override;

    virtual void selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                                      const QList<ModelNode> &lastSelectedNodeList) override;

    virtual void nodeOrderChanged(const NodeListProperty &listProperty, const ModelNode &movedNode, int oldIndex) override;
    virtual void scriptFunctionsChanged(const ModelNode &node, const QStringList &scriptFunctionList) override;
    void instancePropertyChange(const QList<QPair<ModelNode, PropertyName> > &propertyList) override;
    void instancesCompleted(const QVector<ModelNode> &completedNodeList) override;
    void instanceInformationsChange(const QMultiHash<ModelNode, InformationName> &informationChangeHash) override;
    void instancesRenderImageChanged(const QVector<ModelNode> &nodeList) override;
    void instancesPreviewImageChanged(const QVector<ModelNode> &nodeList) override;
    void instancesChildrenChanged(const QVector<ModelNode> &nodeList) override;
    void instancesToken(const QString &tokenName, int tokenNumber, const QVector<ModelNode> &nodeVector) override;

    void nodeSourceChanged(const ModelNode &modelNode, const QString &newNodeSource) override;

    void rewriterBeginTransaction();
    void rewriterEndTransaction();

    void currentStateChanged(const ModelNode &node) override;

    void importsChanged(const QList<Import> &addedImports, const QList<Import> &removedImports) override;

    ModelNode insertModel(const ModelNode &modelNode)
    { return m_modelMerger.insertModel(modelNode); }
    void replaceModel(const ModelNode &modelNode)
    { m_modelMerger.replaceModel(modelNode); }

    void toClipboard() const;
    void fromClipboard();

    QString toText() const;
    void fromText(QString text);

private:
    ModelMerger m_modelMerger;
};

}// namespace QmlDesigner

#endif // DesignDocumentVIEW_H
