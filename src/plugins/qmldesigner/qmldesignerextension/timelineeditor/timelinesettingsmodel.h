/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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
