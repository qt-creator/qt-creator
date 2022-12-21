// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

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

    ComponentView(ExternalDependenciesInterface &externalDependencies);

    void modelAttached(Model *model) override;
    void modelAboutToBeDetached(Model *model) override;

    ComponentAction *action();

    void nodeCreated(const ModelNode &createdNode) override;
    void nodeAboutToBeRemoved(const ModelNode &removedNode) override;
    void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent,
                        const NodeAbstractProperty &oldPropertyParent,
                        AbstractView::PropertyChangeFlags propertyChange) override;
    void nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId) override;
    void nodeSourceChanged(const ModelNode &node, const QString &newNodeSource) override;

    QStandardItemModel *standardItemModel() const;

    ModelNode modelNode(int index) const;

    void setComponentNode(const ModelNode &node);
    void setComponentToMaster();

signals:
    void componentListChanged(const QStringList &componentList);

private: //functions
    void updateModel();
    void searchForComponentAndAddToList(const ModelNode &node);
    void removeFromListRecursive(const ModelNode &node);
    void removeNodeFromList(const ModelNode &node);
    void addNodeToList(const ModelNode &node);
    int indexForNode(const ModelNode &node) const;
    int indexOfMaster() const;
    bool hasMasterEntry() const;
    bool hasEntryForNode(const ModelNode &node) const;
    void ensureMasterDocument();
    void maybeRemoveMasterDocument();
    QString descriptionForNode(const ModelNode &node) const;
    void updateDescription(const ModelNode &node);
    bool isSubComponentNode(const ModelNode &node) const;

private:
    QStandardItemModel *m_standardItemModel;
    ComponentAction *m_componentAction;
};

} // namespace QmlDesigner
