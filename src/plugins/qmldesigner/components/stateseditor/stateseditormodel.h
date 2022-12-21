// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QAbstractListModel>
#include <QPointer>


namespace QmlDesigner {

class StatesEditorView;

class StatesEditorModel : public QAbstractListModel
{
    Q_OBJECT

    enum {
        StateNameRole = Qt::DisplayRole,
        StateImageSourceRole = Qt::UserRole,
        InternalNodeId,
        HasWhenCondition,
        WhenConditionString,
        IsDefault,
        ModelHasDefaultState,
        StateType
    };

public:
    StatesEditorModel(StatesEditorView *view);

    int count() const;
    QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void insertState(int stateIndex);
    void removeState(int stateIndex);
    void updateState(int beginIndex, int endIndex);
    Q_INVOKABLE void renameState(int internalNodeId, const QString &newName);
    Q_INVOKABLE void setWhenCondition(int internalNodeId, const QString &condition);
    Q_INVOKABLE void resetWhenCondition(int internalNodeId);
    Q_INVOKABLE QStringList autoComplete(const QString &text, int pos, bool explicitComplete);
    Q_INVOKABLE QVariant stateModelNode();
    Q_INVOKABLE void setStateAsDefault(int internalNodeId);
    Q_INVOKABLE void resetDefaultState();
    Q_INVOKABLE bool hasDefaultState() const;
    Q_INVOKABLE void setAnnotation(int internalNodeId);
    Q_INVOKABLE void removeAnnotation(int internalNodeId);
    Q_INVOKABLE bool hasAnnotation(int internalNodeId) const;

    void reset();


signals:
    void changedToState(int n);

private:
    QPointer<StatesEditorView> m_statesEditorView;
    int m_updateCounter;
};

} // namespace QmlDesigner
