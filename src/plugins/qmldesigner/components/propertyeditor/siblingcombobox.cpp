/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "siblingcombobox.h"
#include <QDeclarativeComponent>

namespace QmlDesigner {

void SiblingComboBox::setItemNode(const QVariant &itemNode)
{
    
    if (!itemNode.value<ModelNode>().isValid() || !QmlItemNode(itemNode.value<ModelNode>()).hasNodeParent())
        return;
    m_itemNode = itemNode.value<ModelNode>();
    setup();
    emit itemNodeChanged();
}

void SiblingComboBox::registerDeclarativeTypes()
{
    qmlRegisterType<SiblingComboBox>("Bauhaus",1,0,"SiblingComboBox");
}

void SiblingComboBox::setSelectedItemNode(const QVariant &itemNode)
{
    if (itemNode.value<ModelNode>() == m_selectedItemNode)
        return;
    if (!itemNode.value<ModelNode>().isValid())
        return;
    m_selectedItemNode = itemNode.value<ModelNode>();
    setup();
    emit selectedItemNodeChanged();
}

void SiblingComboBox::changeSelection(int i)
{
    if (i < 0 || m_itemList.at(i) == m_selectedItemNode)
        return;
    setSelectedItemNode(QVariant::fromValue(m_itemList.at(i).modelNode()));
}

void SiblingComboBox::setup()
{
    connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(changeSelection(int)));
    if (!m_itemNode.isValid())
        return;

    if (m_itemNode.instanceParent().modelNode().isValid())
        m_itemList = toQmlItemNodeList(m_itemNode.instanceParent().modelNode().allDirectSubModelNodes());
    m_itemList.removeOne(m_itemNode);
    //We currently have no instanceChildren().
    //So we double check here if the instanceParents are equal.
    foreach (const QmlItemNode &node, m_itemList)
        if (node.isValid() && (node.instanceParent().modelNode() != m_itemNode.instanceParent().modelNode()))
            m_itemList.removeAll(node);

    disconnect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(changeSelection(int)));
    clear();

    foreach (const QmlItemNode &itemNode, m_itemList) {
        if (itemNode.isValid()) {
            if (itemNode.id().isEmpty())
                addItem(itemNode.simplifiedTypeName());
            else
                addItem(itemNode.id());
        }
    }

    QmlItemNode parent(m_itemNode.instanceParent().toQmlItemNode());

    if (parent.isValid()) {
        m_itemList.prepend(parent);
        QString parentString("Parent (");

        if (parent.id().isEmpty())
            parentString += parent.simplifiedTypeName();
        else
            parentString += parent.id();
        parentString += ')';
        insertItem(0, parentString);
    }
    setCurrentIndex(m_itemList.indexOf(m_selectedItemNode));
    connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(changeSelection(int)));
}


} //QmlDesigner
