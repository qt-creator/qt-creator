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
    void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent,
                        const NodeAbstractProperty &oldPropertyParent,
                        AbstractView::PropertyChangeFlags propertyChange) override;
    void nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId) override;

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
