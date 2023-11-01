// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmltimeline.h>

#include <QDialog>

QT_BEGIN_NAMESPACE
class QSpinBox;
class QTabWidget;
class QTableView;
QT_END_NAMESPACE

namespace QmlDesigner {

class TimelineForm;
class TimelineAnimationForm;
class TimelineView;
class TimelineSettingsModel;

class TimelineSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TimelineSettingsDialog(QWidget *parent, TimelineView *view);
    void setCurrentTimeline(const QmlTimeline &timeline);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    void setupTableView();
    void setupTimelines(const QmlTimeline &node);
    void setupAnimations(const ModelNode &node);

    void addTimelineTab(const QmlTimeline &node);
    void addAnimationTab(const ModelNode &node);

    TimelineForm *currentTimelineForm() const;

    QTabWidget *m_timelineTab;
    QTabWidget *m_animationTab;
    QTableView *m_tableView;

    TimelineView *m_timelineView;
    QmlTimeline m_currentTimeline;
    TimelineSettingsModel *m_timelineSettingsModel;
};

} // namespace QmlDesigner
