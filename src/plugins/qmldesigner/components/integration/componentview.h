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

#ifndef COMPONENTVIEW_H
#define COMPONENTVIEW_H

#include <abstractview.h>
#include <modelnode.h>

#include <QStringList>

QT_BEGIN_NAMESPACE
class QStandardItemModel;
class QComboBox;
QT_END_NAMESPACE

namespace QmlDesigner {

class ComponentAction;

class ComponentView : public AbstractView
{
    Q_OBJECT

public:
    enum UserRoles
    {
        ModelNodeRole = Qt::UserRole
    };

    ComponentView(QObject *parent = 0);

    void modelAttached(Model *model) override;
    void modelAboutToBeDetached(Model *model) override;

    ComponentAction *action();

    void nodeCreated(const ModelNode &createdNode) override;
    void nodeAboutToBeRemoved(const ModelNode &removedNode) override;
    void nodeRemoved(const ModelNode &removedNode, const NodeAbstractProperty &parentProperty,
                     PropertyChangeFlags propertyChange) override;
    void nodeAboutToBeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent,
                                 const NodeAbstractProperty &oldPropertyParent,
                                 AbstractView::PropertyChangeFlags propertyChange) override;
    void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent,
                        const NodeAbstractProperty &oldPropertyParent,
                        AbstractView::PropertyChangeFlags propertyChange) override;
    void nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId) override;
    void propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList) override;
    void propertiesRemoved(const QList<AbstractProperty>& propertyList) override;
    void variantPropertiesChanged(const QList<VariantProperty>& propertyList,
                                  PropertyChangeFlags propertyChange) override;
    void bindingPropertiesChanged(const QList<BindingProperty>& propertyList,
                                  PropertyChangeFlags propertyChange) override;
    void signalHandlerPropertiesChanged(const QVector<SignalHandlerProperty>& propertyList,
                                        PropertyChangeFlags propertyChange) override;
    void rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion) override;
    void scriptFunctionsChanged(const ModelNode &node, const QStringList &scriptFunctionList) override;
    void instancePropertyChange(const QList<QPair<ModelNode, PropertyName> > &propertyList) override;
    void instancesCompleted(const QVector<ModelNode> &completedNodeList) override;
    void instanceInformationsChange(const QMultiHash<ModelNode, InformationName> &informationChangeHash) override;
    void instancesRenderImageChanged(const QVector<ModelNode> &nodeList) override;
    void instancesPreviewImageChanged(const QVector<ModelNode> &nodeList) override;
    void instancesChildrenChanged(const QVector<ModelNode> &nodeList) override;
    void instancesToken(const QString &tokenName, int tokenNumber, const QVector<ModelNode> &nodeVector) override;

    void nodeSourceChanged(const ModelNode &modelNode, const QString &newNodeSource) override;

    void rewriterBeginTransaction() override;
    void rewriterEndTransaction() override;

    void currentStateChanged(const ModelNode &node) override;

    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                                      const QList<ModelNode> &lastSelectedNodeList) override;

    void fileUrlChanged(const QUrl &oldUrl, const QUrl &newUrl) override;

    void nodeOrderChanged(const NodeListProperty &listProperty, const ModelNode &movedNode, int oldIndex) override;

    void auxiliaryDataChanged(const ModelNode &node, const PropertyName &name, const QVariant &data) override;

    void customNotification(const AbstractView *view, const QString &identifier,
                            const QList<ModelNode> &nodeList, const QList<QVariant> &data) override;

    void importsChanged(const QList<Import> &addedImports, const QList<Import> &removedImports) override;

    QStandardItemModel *standardItemModel() const;

    ModelNode modelNode(int index) const;

    void setComponentNode(const ModelNode &node);
    void setComponentToMaster();

signals:
    void componentListChanged(const QStringList &componentList);

private: //functions
    void updateModel();
    void searchForComponentAndAddToList(const ModelNode &node);
    void searchForComponentAndRemoveFromList(const ModelNode &node);
    void removeSingleNodeFromList(const ModelNode &node);
    int indexForNode(const ModelNode &node) const;
    int indexOfMaster() const;
    bool hasMasterEntry() const;
    bool hasEntryForNode(const ModelNode &node) const;
    void addMasterDocument();
    void removeMasterDocument();
    QString descriptionForNode(const ModelNode &node) const;
    void updateDescription(const ModelNode &node);

private:
    QStandardItemModel *m_standardItemModel;
    ComponentAction *m_componentAction;
};

} // namespace QmlDesigner

#endif // COMPONENTVIEW_H
