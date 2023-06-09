// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QAbstractListModel>
#include <QPointer>

namespace QmlDesigner {

class StatesEditorView;

class StatesEditorModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(bool canAddNewStates READ canAddNewStates WRITE setCanAddNewStates NOTIFY
                   canAddNewStatesChanged)

    enum {
        StateNameRole = Qt::DisplayRole,
        StateImageSourceRole = Qt::UserRole,
        InternalNodeId,
        HasWhenCondition,
        WhenConditionString,
        IsDefault,
        ModelHasDefaultState,
        HasExtend,
        ExtendString
    };

public:
    StatesEditorModel(StatesEditorView *view);

    Q_INVOKABLE int count() const;
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
    Q_INVOKABLE QVariant stateModelNode(int internalNodeId);

    Q_INVOKABLE void setStateAsDefault(int internalNodeId);
    Q_INVOKABLE void resetDefaultState();
    Q_INVOKABLE bool hasDefaultState() const;
    Q_INVOKABLE void setAnnotation(int internalNodeId);
    Q_INVOKABLE void removeAnnotation(int internalNodeId);
    Q_INVOKABLE bool hasAnnotation(int internalNodeId) const;

    QStringList stateGroups() const;
    QString activeStateGroup() const;
    void setActiveStateGroup(const QString &name);
    int activeStateGroupIndex() const;
    void setActiveStateGroupIndex(int index);

    Q_INVOKABLE bool renameActiveStateGroup(const QString &name);

    Q_INVOKABLE void addStateGroup(const QString &name);
    Q_INVOKABLE void removeStateGroup();

    Q_INVOKABLE QVariantMap get(int idx) const;

    QVariantMap baseState() const;
    bool hasExtend() const;
    QStringList extendedStates() const;

    Q_PROPERTY(QVariantMap baseState READ baseState NOTIFY baseStateChanged)
    Q_PROPERTY(QStringList extendedStates READ extendedStates NOTIFY extendedStatesChanged)

    Q_PROPERTY(bool hasExtend READ hasExtend NOTIFY hasExtendChanged)

    Q_PROPERTY(QString activeStateGroup READ activeStateGroup WRITE setActiveStateGroup NOTIFY
                   activeStateGroupChanged)
    Q_PROPERTY(int activeStateGroupIndex READ activeStateGroupIndex WRITE setActiveStateGroupIndex
                   NOTIFY activeStateGroupIndexChanged)
    Q_PROPERTY(QStringList stateGroups READ stateGroups NOTIFY stateGroupsChanged)

    Q_INVOKABLE void move(int from, int to);
    Q_INVOKABLE void drop(int from, int to);

    void reset();
    void evaluateExtend();

    bool canAddNewStates() const;
    void setCanAddNewStates(bool b);

signals:
    void changedToState(int n);
    void baseStateChanged();
    void hasExtendChanged();
    void extendedStatesChanged();
    void activeStateGroupChanged();
    void activeStateGroupIndexChanged();
    void stateGroupsChanged();
    void canAddNewStatesChanged();

private:
    QPointer<StatesEditorView> m_statesEditorView;
    bool m_hasExtend;
    QStringList m_extendedStates;
    bool m_canAddNewStates = false;
};

} // namespace QmlDesigner
