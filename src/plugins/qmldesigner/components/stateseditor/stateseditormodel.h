/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef STATESEDITORMODEL_H
#define STATESEDITORMODEL_H

#include <QAbstractListModel>
#include <QWeakPointer>

#include <stateseditorview.h>

namespace QmlDesigner {

namespace Internal {

class StatesEditorModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int count READ count NOTIFY countChanged)

    enum {
        StateNameRole = Qt::DisplayRole,
        StateImageSourceRole = Qt::UserRole,
    };

public:
    StatesEditorModel(QObject *parent);

    int count() const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    void insertState(int i, const QString &name);
    void removeState(int i);
    Q_INVOKABLE void renameState(int i, const QString &newName);
    void updateState(int i);
    void setStatesEditorView(StatesEditorView *statesView);
    void emitChangedToState(int n);

signals:
    void countChanged();
    void changedToState(int n);

private:
    QList<QString> m_stateNames;
    QWeakPointer<StatesEditorView> m_statesView;
    int m_updateCounter;
};

} // namespace Itnernal
} // namespace QmlDesigner

#endif // STATESEDITORMODEL_H
