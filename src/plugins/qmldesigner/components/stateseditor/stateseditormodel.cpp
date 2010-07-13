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

enum {
    debug = false
};


namespace QmlDesigner {
namespace Internal {

StatesEditorModel::StatesEditorModel(QObject *parent) :
        QAbstractListModel(parent),
        m_updateCounter(0)
{
    QHash<int, QByteArray> roleNames;
    roleNames.insert(StateNameRole, "stateName");
    roleNames.insert(StateImageSourceRole, "stateImageSource");
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
                result = QString(tr("base state", "Implicit default state"));
            else
                result = m_stateNames.at(index.row());
            break;
        }
    case StateImageSourceRole: {
            if (!m_statesView.isNull()) 
                return QString("image://qmldesigner_stateseditor/%1-%2").arg(index.row()).arg(m_updateCounter);
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
        if (m_stateNames.contains(newName) || newName.isEmpty()) {
            QMessageBox::warning(0, tr("Invalid state name"), newName.isEmpty()?tr("The empty string as a name is reserved for the base state."):tr("Name already used in another state"));
        } else {
            m_stateNames.replace(i, newName);
            m_statesView->renameState(i,newName);

            emit dataChanged(createIndex(i, 0), createIndex(i, 0));
        }
    }
}

void StatesEditorModel::updateState(int i)
{
    Q_ASSERT(i >= 0 && i < m_stateNames.count());

    // QML images with the same URL are always cached, so this changes the URL each 
    // time to ensure the image is loaded from the StatesImageProvider and not the 
    // cache.
    // TODO: only increase imageId when the scene has changed so that we can load 
    // from the cache instead where possible.
    if (++m_updateCounter == INT_MAX)
        m_updateCounter = 0;

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
