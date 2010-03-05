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

enum {
    debug = false
};


namespace QmlDesigner {
namespace Internal {

StatesEditorModel::StatesEditorModel(QObject *parent) :
        QAbstractListModel(parent)
{

    QHash<int, QByteArray> roleNames;
    roleNames.insert(StateNameRole, "stateName");
    roleNames.insert(StatesPixmapRole, "statePixmap");
    setRoleNames(roleNames);
}


int StatesEditorModel::count() const
{
    return m_stateNames.count();
}

int StatesEditorModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_stateNames.count();
}

QVariant StatesEditorModel::data(const QModelIndex &index, int role) const
{
    if (index.parent().isValid() || index.column() != 0)
        return QVariant();

    QVariant result;
    switch (role) {
    case StateNameRole: {
            if (index.row()==0)
                result = QString("base state");
            else
                result = m_stateNames.at(index.row());
            break;
        }
    case StatesPixmapRole: {
            // TODO: Maybe cache this?
            if (!m_statesView.isNull()) {
                result = m_statesView->renderState(index.row());
            }
            break;
        }
    }

    return result;
}

void StatesEditorModel::insertState(int i, const QString &name)
{
    beginInsertRows(QModelIndex(), i, i);

    m_stateNames.insert(i, name);

    endInsertRows();

    emit dataChanged(createIndex(i, 0), createIndex(i, 0));
    emit countChanged();
}

void StatesEditorModel::removeState(int i)
{
    beginRemoveRows(QModelIndex(), i, i);

    m_stateNames.removeAt(i);

    endRemoveRows();

    emit dataChanged(createIndex(i, 0), createIndex(i, 0));
    emit countChanged();
}

void StatesEditorModel::renameState(int i, const QString &newName)
{
    Q_ASSERT(i > 0 && i < m_stateNames.count());

    if (m_stateNames[i] != newName) {
        m_stateNames.replace(i, newName);
        m_statesView->renameState(i,newName);

        emit dataChanged(createIndex(i, 0), createIndex(i, 0));
    }
}

void StatesEditorModel::updateState(int i)
{
    Q_ASSERT(i >= 0 && i < m_stateNames.count());

    emit dataChanged(createIndex(i, 0), createIndex(i, 0));
}

void StatesEditorModel::setStatesEditorView(StatesEditorView *statesView)
{
    m_statesView = statesView;
}

void StatesEditorModel::emitChangedToState(int n)
{
    emit changedToState(n);
}

} // namespace Internal
} // namespace QmlDesigner
