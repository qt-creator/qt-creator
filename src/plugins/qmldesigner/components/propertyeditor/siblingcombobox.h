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

#ifndef SIBLINGCOMBOBOX_H
#define SIBLINGCOMBOBOX_H

#include <QComboBox>

#include <qmlitemnode.h>

namespace QmlDesigner {

class SiblingComboBox : public QComboBox
{
    Q_OBJECT
    Q_PROPERTY(QVariant itemNode READ itemNode WRITE setItemNode NOTIFY itemNodeChanged)
    Q_PROPERTY(QVariant selectedItemNode READ selectedItemNode WRITE setSelectedItemNode NOTIFY selectedItemNodeChanged)

public:
    SiblingComboBox(QWidget *parent = 0) : QComboBox(parent) { }

    QVariant itemNode() const { return QVariant::fromValue(m_itemNode.modelNode()); }
    QVariant selectedItemNode() const { return QVariant::fromValue(m_selectedItemNode.modelNode()); }

    void setItemNode(const QVariant &itemNode);
    void setSelectedItemNode(const QVariant &itemNode);

    static void registerDeclarativeTypes();

signals:
    void selectedItemNodeChanged();
    void itemNodeChanged();

private slots:
    void changeSelection(int i);

private:
    void setup();
    QmlItemNode m_itemNode;
    QmlItemNode m_selectedItemNode;
    QList<QmlItemNode> m_itemList;


};

}

#endif //SIBLINGCOMBOBOX_H

