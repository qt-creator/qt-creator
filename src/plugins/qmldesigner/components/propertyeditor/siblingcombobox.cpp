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
    QML_REGISTER_TYPE(Bauhaus,1,0,SiblingComboBox,SiblingComboBox);
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
    connect(this, SIGNAL(currentIndexChanged (int)), this, SLOT(changeSelection(int)));
    if (!m_itemNode.isValid())
        return;
    m_itemList = m_itemNode.instanceParent().toQmlItemNode().children();
    m_itemList.removeOne(m_itemNode);
    

    disconnect(this, SIGNAL(currentIndexChanged (int)), this, SLOT(changeSelection(int)));
    clear();

    foreach (const QmlItemNode &itemNode, m_itemList) {
        if (itemNode.isValid()) {
            if (itemNode.id().isEmpty())
                addItem(itemNode.simplfiedTypeName());
            else
                addItem(itemNode.id());
        }
    }

    QmlItemNode parent(m_itemNode.instanceParent().toQmlItemNode());
    m_itemList.prepend(parent);
    QString parentString("Parent (");

    if (parent.id().isEmpty())
        parentString += parent.simplfiedTypeName();
    else
        parentString += parent.id();
    parentString += ")";
    insertItem(0, parentString);

    setCurrentIndex(m_itemList.indexOf(m_selectedItemNode));
    connect(this, SIGNAL(currentIndexChanged (int)), this, SLOT(changeSelection(int)));

}


} //QmlDesigner
