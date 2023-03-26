// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmltimeline.h>

#include <QAbstractItemView>
#include <QStandardItemModel>

namespace QmlDesigner {

class ModelNode;
class BindingProperty;
class SignalHandlerProperty;
class VariantProperty;

class TimelineView;

class TimelineSettingsModel : public QStandardItemModel
{
    Q_OBJECT
public:
    enum ColumnRoles { StateRow = 0, TimelineRow = 1, AnimationRow = 2, FixedFrameRow = 3 };
    TimelineSettingsModel(QObject *parent, TimelineView *view);
    void resetModel();

    TimelineView *timelineView() const;

    QStringList getSignalsForRow(int row) const;
    ModelNode getTargetNodeForConnection(const ModelNode &connection) const;

    void addConnection();

    void bindingPropertyChanged(const BindingProperty &bindingProperty);
    void variantPropertyChanged(const VariantProperty &variantProperty);

    void setupDelegates(QAbstractItemView *view);

    QmlTimeline timelineForRow(int row) const;
    ModelNode animationForRow(int row) const;
    ModelNode stateForRow(int row) const;
    int fixedFrameForRow(int row) const;

protected:
    void addState(const ModelNode &modelNode);

private:
    void handleDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void handleException();
    ModelNode animationForTimelineAndState(const QmlTimeline &timeline, const ModelNode &state);

    void updateTimeline(int row);
    void updateAnimation(int row);
    void updateFixedFrameRow(int row);

    void resetRow(int row);

private:
    TimelineView *m_timelineView;
    bool m_lock = false;
    QString m_exceptionError;
};

} // namespace QmlDesigner
