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

#include "timelineutils.h"
#include <coreplugin/icontext.h>

#include <QWidget>

#include <functional>

QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QGraphicsView)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QResizeEvent)
QT_FORWARD_DECLARE_CLASS(QScrollBar)
QT_FORWARD_DECLARE_CLASS(QShowEvent)
QT_FORWARD_DECLARE_CLASS(QString)
QT_FORWARD_DECLARE_CLASS(QPushButton)

namespace QmlDesigner {

class TimelineToolBar;
class TimelineView;
class TimelineGraphicsScene;
class QmlTimeline;

class TimelineWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TimelineWidget(TimelineView *view);
    void contextHelp(const Core::IContext::HelpCallback &callback) const;

    TimelineGraphicsScene *graphicsScene() const;
    TimelineView *timelineView() const;
    TimelineToolBar *toolBar() const;

    void init();
    void reset();

    void invalidateTimelineDuration(const QmlTimeline &timeline);
    void invalidateTimelinePosition(const QmlTimeline &timeline);
    void setupScrollbar(int min, int max, int current);
    void setTimelineId(const QString &id);

    void setTimelineActive(bool b);

public slots:
    void selectionChanged();
    void openEasingCurveEditor();
    void setTimelineRecording(bool value);
    void changeScaleFactor(int factor);
    void scroll(const TimelineUtils::Side &side);

protected:
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void connectToolbar();

    int adjacentFrame(const std::function<qreal(const QVector<qreal> &, qreal)> &fun) const;

    TimelineToolBar *m_toolbar = nullptr;

    QGraphicsView *m_rulerView = nullptr;

    QGraphicsView *m_graphicsView = nullptr;

    QScrollBar *m_scrollbar = nullptr;

    QLabel *m_statusBar = nullptr;

    TimelineView *m_timelineView = nullptr;

    TimelineGraphicsScene *m_graphicsScene;

    QPushButton *m_addButton = nullptr;
};

} // namespace QmlDesigner
