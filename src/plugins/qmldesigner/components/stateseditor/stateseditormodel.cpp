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

#include "stateseditormodel.h"
#include "stateseditorview.h"

#include <QDebug>

#include <nodelistproperty.h>
#include <modelnode.h>
#include <bindingproperty.h>
#include <variantproperty.h>
#include <rewriterview.h>

#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>

enum {
    debug = false
};


namespace QmlDesigner {

StatesEditorModel::StatesEditorModel(StatesEditorView *view)
    : QAbstractListModel(view),
      m_statesEditorView(view),
      m_updateCounter(0)
{
}


int StatesEditorModel::count() const
{
    return rowCount();
}

QModelIndex StatesEditorModel::index(int row, int column, const QModelIndex &parent) const
{
    if (m_statesEditorView.isNull())
        return QModelIndex();


    int internalNodeId = 0;
    if (row > 0)
        internalNodeId = m_statesEditorView->rootModelNode().nodeListProperty("states").at(row - 1).internalId();

    return hasIndex(row, column, parent) ? createIndex(row, column,  internalNodeId) : QModelIndex();
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
    QAbstractListModel::beginResetModel();
    QAbstractListModel::endResetModel();
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
    case InternalNodeId: return index.internalId();

    case HasWhenCondition: return stateNode.isValid() && stateNode.hasProperty("when");

    case WhenConditionString: {
        if (stateNode.isValid() && stateNode.hasBindingProperty("when"))
            return stateNode.bindingProperty("when").expression();
        else
            return QString();
    }

    }

    return QVariant();
}

QHash<int, QByteArray> StatesEditorModel::roleNames() const
{
    static QHash<int, QByteArray> roleNames{
        {StateNameRole, "stateName"},
        {StateImageSourceRole, "stateImageSource"},
        {InternalNodeId, "internalNodeId"},
        {HasWhenCondition, "hasWhenCondition"},
        {WhenConditionString, "whenConditionString"}
    };
    return roleNames;
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

void StatesEditorModel::renameState(int internalNodeId, const QString &newName)
{
    if (newName == m_statesEditorView->currentStateName())
        return;

    if (newName.isEmpty() ||! m_statesEditorView->validStateName(newName)) {
        Core::AsynchronousMessageBox::warning(tr("Invalid state name"),
                                               newName.isEmpty() ?
                                                   tr("The empty string as a name is reserved for the base state.") :
                                                   tr("Name already used in another state"));
    } else {
        m_statesEditorView->renameState(internalNodeId, newName);
    }

}

void StatesEditorModel::setWhenCondition(int internalNodeId, const QString &condition)
{
    m_statesEditorView->setWhenCondition(internalNodeId, condition);
}

void StatesEditorModel::resetWhenCondition(int internalNodeId)
{
    m_statesEditorView->resetWhenCondition(internalNodeId);
}

QStringList StatesEditorModel::autoComplete(const QString &text, int pos, bool explicitComplete)
{
    Model *model = m_statesEditorView->model();
    if (model && model->rewriterView())
        return model->rewriterView()->autoComplete(text, pos, explicitComplete);

    return QStringList();
}

} // namespace QmlDesigner
