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

#ifndef STATESEDITORMODEL_H
#define STATESEDITORMODEL_H

#include <QAbstractListModel>
#include <QWeakPointer>


namespace QmlDesigner {

class StatesEditorView;

class StatesEditorModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int count READ count NOTIFY countChanged)

    enum {
        StateNameRole = Qt::DisplayRole,
        StateImageSourceRole = Qt::UserRole,
        NodeId
    };

public:
    StatesEditorModel(StatesEditorView *view);

    int count() const;
    QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    void insertState(int stateIndex);
    void removeState(int stateIndex);
    void updateState(int stateIndex);
    Q_INVOKABLE void renameState(int nodeId, const QString &newName);
    void emitChangedToState(int n);

    void reset();

signals:
    void countChanged();
    void changedToState(int n);

private:
    QWeakPointer<StatesEditorView> m_statesEditorView;
    int m_updateCounter;
};

} // namespace QmlDesigner

#endif // STATESEDITORMODEL_H
