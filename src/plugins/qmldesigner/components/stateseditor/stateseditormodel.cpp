/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "stateseditormodel.h"
#include "stateseditorview.h"

#include <QDebug>
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
                if (stateNode.hasVariantProperty("name"))
                    return stateNode.variantProperty("name").value();
                else
                    return QVariant();
            }

        }
    case StateImageSourceRole: {
        static int randomNumber = 0;
        randomNumber++;
        if (index.row() == 0)
            return QString("image://qmldesigner_stateseditor/baseState-%1").arg(randomNumber);
        else
            return QString("image://qmldesigner_stateseditor/%1-%2").arg(index.internalId()).arg(randomNumber);
    }
    case NodeId : return index.internalId();
    }


    return QVariant();
}

void StatesEditorModel::insertState(int stateIndex)
{
    if (stateIndex >= 0) {

        const int updateIndex = stateIndex + 1;
        beginInsertRows(QModelIndex(), updateIndex, updateIndex);

        endInsertRows();

        emit dataChanged(index(updateIndex, 0), index(updateIndex, 0));
        emit countChanged();
    }
}

void StatesEditorModel::updateState(int beginIndex, int endIndex)
{
    if (beginIndex >= 0 && endIndex >= 0)
        emit dataChanged(index(beginIndex, 0), index(endIndex, 0));
}

void StatesEditorModel::removeState(int stateIndex)
{
    if (stateIndex >= 0) {
        const int updateIndex = stateIndex + 1;
        beginRemoveRows(QModelIndex(), updateIndex, updateIndex);


        endRemoveRows();

        emit dataChanged(createIndex(updateIndex, 0), createIndex(updateIndex, 0));
        emit countChanged();
    }
}

void StatesEditorModel::renameState(int nodeId, const QString &newName)
{
    if (newName == m_statesEditorView->currentStateName())
        return;

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
