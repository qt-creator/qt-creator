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

#include "stateseditormodel.h"
#include "stateseditorview.h"

#include <QtCore/QDebug>
#include <QMessageBox>

#include <nodelistproperty.h>
#include <modelnode.h>
#include <variantproperty.h>

enum {
    debug = false
};


namespace QmlDesigner {

StatesEditorModel::StatesEditorModel(StatesEditorView *view)
    : QAbstractListModel(view),
      m_statesEditorView(view),
      m_updateCounter(0)
{
    QHash<int, QByteArray> roleNames;
    roleNames.insert(StateNameRole, "stateName");
    roleNames.insert(StateImageSourceRole, "stateImageSource");
    roleNames.insert(NodeId, "nodeId");
    setRoleNames(roleNames);
}


int StatesEditorModel::count() const
{
    return rowCount();
}

QModelIndex StatesEditorModel::index(int row, int column, const QModelIndex &parent) const
{
    if (m_statesEditorView.isNull())
        return QModelIndex();


    int internalId = 0;
    if (row > 0)
        internalId = m_statesEditorView->rootModelNode().nodeListProperty("states").at(row - 1).internalId();

    return hasIndex(row, column, parent) ? createIndex(row, column,  internalId) : QModelIndex();
}

int StatesEditorModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid() || m_statesEditorView.isNull() || !m_statesEditorView->model())
        return 0;

    if (!m_statesEditorView->rootModelNode().hasNodeListProperty("states"))
        return 1;

    return m_statesEditorView->rootModelNode().nodeListProperty("states").count() + 1;
}

void StatesEditorModel::reset()
{
    QAbstractListModel::reset();
}

QVariant StatesEditorModel::data(const QModelIndex &index, int role) const
{
    if (index.parent().isValid() || index.column() != 0 || m_statesEditorView.isNull() || !m_statesEditorView->hasModelNodeForInternalId(index.internalId()))
        return QVariant();

    ModelNode stateNode;

    if (index.internalId() > 0)
        stateNode = m_statesEditorView->modelNodeForInternalId(index.internalId());

    switch (role) {
    case StateNameRole: {
            if (index.row() == 0) {
                return QString(tr("base state", "Implicit default state"));
            } else {
                if (stateNode.hasVariantProperty("name")) {
                    return stateNode.variantProperty("name").value();
                } else {
                    return QVariant();
                }
            }

        }
    case StateImageSourceRole: return QString("image://qmldesigner_stateseditor/%1").arg(index.internalId());
    case NodeId : return index.internalId();
    }


    return QVariant();
}

void StatesEditorModel::insertState(int stateIndex)
{
    if (stateIndex >= 0) {

        const int index = stateIndex + 1;
        beginInsertRows(QModelIndex(), index, index);

        endInsertRows();

        emit dataChanged(createIndex(index, 0), createIndex(index, 0));
        emit countChanged();
    }
}

void StatesEditorModel::updateState(int stateIndex)
{
    if (stateIndex >= 0) {
        const int index = stateIndex + 1;

        emit dataChanged(createIndex(index, 0), createIndex(index, 0));
    }
}

void StatesEditorModel::removeState(int stateIndex)
{
    if (stateIndex >= 0) {
        const int index = stateIndex + 1;
        beginRemoveRows(QModelIndex(), index, index);


        endRemoveRows();

        emit dataChanged(createIndex(index, 0), createIndex(index, 0));
        emit countChanged();
    }
}

void StatesEditorModel::renameState(int nodeId, const QString &newName)
{    
    if (newName.isEmpty() ||! m_statesEditorView->validStateName(newName)) {
        QMessageBox::warning(0, tr("Invalid state name"),
                             newName.isEmpty() ?
                                 tr("The empty string as a name is reserved for the base state.") :
                                 tr("Name already used in another state"));
    } else {
        m_statesEditorView->renameState(nodeId, newName);
    }

}

void StatesEditorModel::emitChangedToState(int n)
{
    emit changedToState(n);
}

} // namespace QmlDesigner
